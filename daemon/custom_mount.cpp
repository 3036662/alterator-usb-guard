#include "custom_mount.hpp"
#include "dal/dto.hpp"
#include "dal/local_storage.hpp"
#include "utils.hpp"
#include <acl/libacl.h>
#include <boost/algorithm/string/predicate.hpp>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <grp.h>
#include <memory>
#include <mntent.h>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <sys/acl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace usbmount {

CustomMount::CustomMount(std::shared_ptr<UsbUdevDevice> &ptr_device,
                         const std::shared_ptr<spdlog::logger> &logger) noexcept
    : logger_(logger), ptr_device_{ptr_device},
      dbase_(dal::LocalStorage::GetStorage()) {}

bool CustomMount::Mount() noexcept {
  // get permissions for this device
  dal::Device dto_device(
      {ptr_device_->vid(), ptr_device_->pid(), ptr_device_->serial()});
  auto db_index = dbase_->permissions.Find(dto_device);
  if (!db_index.has_value())
    return false;
  auto perms = dbase_->permissions.Read(db_index.value());
  // for now, only one user and group is allowed, though db stores users and
  // groups as array
  const auto &users = perms.getUsers();
  const auto &groups = perms.getGroups();
  if (users.empty() || groups.empty()) {
    logger_->error("Empty user or group in permissions for device {}",
                   ptr_device_->block_name());
    return false;
  }
  uid_ = users[0].uid();
  gid_ = groups[0].gid();
  logger_->debug(ptr_device_->toString());
  if (ptr_device_->dev_type() == "disk" &&
      ptr_device_->filesystem() != "ntfs") {
    logger_->debug("Skipped with device type disk");
    return true;
  }
  if (!CreateAclMountPoint())
    return false;
  // create endpoint
  if (!CreateMountEndpoint())
    return false;
  // mount and save to local storage
  if (PerfomMount()) {
    try {
      dal::MountEntry entry(dal::MountEntryParams(
          {ptr_device_->block_name(), end_mount_point_.value_or(""),
           ptr_device_->filesystem()}));
      dbase_->mount_points.Create(entry);
      logger_->debug("Created mountpouint for {} in the db",
                     ptr_device_->block_name());
    } catch (const std::exception &ex) {
      logger_->error("Can't  add {} device mountpoint to database",
                     ptr_device_->block_name());
    }
  }
  return true;
}

bool CustomMount::CreateAclMountPoint() noexcept {
  std::string mount_point = mount_root;
  // get user name
  std::optional<std::string> user_name;
  const passwd *pwd = getpwuid(uid_.value_or(0));
  if (pwd->pw_name != NULL)
    user_name = pwd->pw_name;
  mount_point += user_name.value_or(std::to_string(uid_.value_or(0)));
  mount_point += '_';
  // get group name
  std::optional<std::string> group_name;
  const group *grp = getgrgid(gid_.value_or(0));
  if (grp->gr_name != NULL)
    group_name = grp->gr_name;
  mount_point += group_name.value_or(std::to_string(gid_.value_or(0)));
  // create acl dir if no exists
  try {
    std::filesystem::create_directories(mount_point);
    if (chmod(mount_point.c_str(), 0750) != 0)
      throw std::runtime_error("Chmod 0750 failed");
    // process ACL
    SetAcl(mount_point);

  } catch (const std::exception &ex) {
    logger_->error("Cant create mount point {}", mount_point);
    logger_->error(ex.what());
    return false;
  }
  base_mount_point_ = std::move(mount_point);
  return true;
}

void CustomMount::SetAcl(const std::string &mount_point) {
  // read acl
  using namespace utils::acl;
  acl_t acl = acl_get_file(mount_point.c_str(), ACL_TYPE_ACCESS);
  acl_t acl_old = acl_get_file(mount_point.c_str(), ACL_TYPE_ACCESS);
  if (acl == NULL)
    throw std::runtime_error("Cant read ACL for mountpoint");
  // delete all ACL_USER and ACL_GROUP
  DeleteACLUserGroupMask(acl);
  acl_entry_t entry;
  acl_permset_t permset;
  try {
    // add ACL_USER
    CreateUserAclEntry(acl, uid_.value_or(0));
    // Add ACL_GROUP
    CreateGroupAclEntry(acl, gid_.value_or(0));
    // Add ACL_MASK
    if (acl_create_entry(&acl, &entry) != 0)
      throw std::runtime_error("Can't create ACL entry for mask");
    if (acl_get_permset(entry, &permset) != 0)
      throw std::runtime_error("acl_get_permset failed");
    if (acl_add_perm(permset, ACL_READ | ACL_EXECUTE) != 0)
      throw std::runtime_error("acl_add_perm failed");
    if (acl_set_tag_type(entry, ACL_MASK) != 0)
      throw std::runtime_error("acl_set_tag_type failed");
    if (acl_set_permset(entry, permset) != 0)
      throw std::runtime_error("acl_set_permset failed");
    logger_->debug(ToString(acl));
    if (acl_valid(acl) != 0) {
      logger_->warn("Acl is invalid");
      int last = 0;
      logger_->warn(acl_error(acl_check(acl, &last)));
      logger_->warn("last entry = {}", last);
    }
    bool skip_acl = acl_cmp(acl, acl_old) != 1;
    if (skip_acl)
      logger_->debug("Skipped setting same ACL");
    if (!skip_acl &&
        acl_set_file(mount_point.c_str(), ACL_TYPE_ACCESS, acl) != 0) {
      logger_->error(strerror(errno));
      throw std::runtime_error("acl_set_file failed");
    }
  } catch (const std::exception &ex) {
    // free acl and rethrow
    acl_free(acl);
    acl_free(acl_old);
    throw;
  }
  // free ACL
  if (acl_free(acl) != 0 || acl_free(acl_old) != 0)
    logger_->warn("acl_free failed");
  logger_->debug("ACL for {} successfully set", mount_point);
}

bool CustomMount::CreateMountEndpoint() noexcept {
  namespace fs = std::filesystem;
  std::string endpoint;
  if (!base_mount_point_ || base_mount_point_->empty()) {
    logger_->error("invalid base mount path");
    return false;
  }
  endpoint += base_mount_point_.value();
  endpoint += "/";
  try {
    if (!ptr_device_->fs_label().empty() &&
        !fs::exists(endpoint + ptr_device_->fs_label())) {
      endpoint += ptr_device_->fs_label();
    } else if (!ptr_device_->fs_uid().empty()) {
      endpoint += ptr_device_->fs_uid();
    } else { // just in case
      endpoint += "usb";
    }
    // check if it exists add some index in this case
    uint index = 0;
    // if exists and non empty -> change name
    while (fs::exists(endpoint) && !fs::is_empty(endpoint)) {
      if (index == 0) {
        endpoint += "_0";
        continue;
      }
      ++index;
      endpoint.erase(endpoint.size() - 1);
      endpoint += std::to_string(index);
    }
    if (!fs::exists(endpoint) && !fs::create_directory(endpoint)) {
      logger_->error("Can't create directory {}", endpoint);
      return false;
    }
    if (chown(endpoint.c_str(), uid_.value_or(0), gid_.value_or(0)) != 0) {
      logger_->error("Chown for failed {} ", endpoint);
      return false;
    }
    if (chmod(endpoint.c_str(), 0750) != 0) {
      logger_->error("Chmod 0750 failed for {}", endpoint);
      return false;
    }
    logger_->info("Created mount endpoint {}", endpoint);
    end_mount_point_ = std::move(endpoint);
  } catch (const std::exception &ex) {
    logger_->error(ex.what());
    return false;
  }
  return true;
}

bool CustomMount::PerfomMount() noexcept {
  if (!end_mount_point_.has_value() ||
      !std::filesystem::exists(end_mount_point_.value())) {
    logger_->error("No endpoint for mount");
    return false;
  }
  // setup mount options
  MountOptions mount_opts{0, ptr_device_->filesystem(), ""};
  SetMountOptions(mount_opts);
  // perfom mount
  int res = mount(ptr_device_->block_name().c_str(),
                  end_mount_point_.value().c_str(), mount_opts.fs.c_str(),
                  mount_opts.mount_flags, mount_opts.mount_data.c_str());
  if (res == 0) {
    logger_->info("Mounted {} to {}", ptr_device_->block_name(),
                  end_mount_point_.value());
  } else {
    logger_->error("[PerfomMount]{}", strerror(errno));
    logger_->info("Try to mount as READ only");
    // if mount failed try to mount as readonly
    res = mount(ptr_device_->block_name().c_str(),
                end_mount_point_.value().c_str(), mount_opts.fs.c_str(),
                mount_opts.mount_flags | MS_RDONLY,
                mount_opts.mount_data.c_str());
    if (res == -1) {
      logger_->error("[PerfomMount] Error mounting as read-only");
      RemoveMountPoint(end_mount_point_.value());
      return false;
    }
    logger_->debug("[PerfomMount] Mounted as READ ONLY");
  }
  // chown+chmod after mount if not read-only fs
  // if (!mount_opts.read_only) {
  //   if (chown(end_mount_point_.value().c_str(), uid_.value_or(0),
  //             gid_.value_or(0)) != 0) {
  //     logger_->error("Chown for {} failed", end_mount_point_.value());
  //     logger_->error(strerror(errno));
  //     return false;
  //   }
  //   if (chmod(end_mount_point_.value().c_str(), 0770) != 0) {
  //     logger_->error("Chmod 0750 failed for {}", end_mount_point_.value());
  //     return false;
  //   }
  // }
  return true;
}

bool CustomMount::UnMount() noexcept {
  // find mount point
  FILE *p_file = setmntent("/etc/mtab", "r");
  if (p_file == NULL) {
    logger_->error("[UnMount] Error opening /etc/mtab");
    return false;
  }
  std::string mount_point;
  std::string fs_type;
  {
    mntent *entry;
    while ((entry = getmntent(p_file)) != NULL) {
      if (std::string(entry->mnt_fsname) == ptr_device_->block_name()) {
        mount_point = entry->mnt_dir;
        fs_type = entry->mnt_type;
        break;
      }
    }
  }
  // just "ntfs" for ntfs3
  if (fs_type == "ntfs3")
    fs_type.pop_back();
  endmntent(p_file);
  // perfom unmount
  if (!mount_point.empty()) {
    int res = umount2(mount_point.c_str(), 0);
    if (res != 0) {
      logger_->error("[UnMount] Error unmounting {}",
                     ptr_device_->block_name());
      logger_->error(std::strerror(errno));
      return false;
    }
  } else {
    logger_->info("[UnMount] {} is not mounted", ptr_device_->block_name());
  }

  // remove from the database
  try {
    dal::MountEntry entry(dal::MountEntryParams(
        {ptr_device_->block_name(), mount_point, fs_type}));
    // TODO find mount point
    auto index = dbase_->mount_points.Find(entry);
    if (index) {
      // remove mount directory
      RemoveMountPoint(mount_point);
      // remove from db
      dbase_->mount_points.Delete(index.value());
      logger_->debug("[UnMount] Deleted {} from mountpoints table",
                     ptr_device_->block_name());
    }
  } catch (const std::exception &ex) {
    logger_->error("[UnMount] Can't  remove {} device mountpoint to database",
                   ptr_device_->block_name());
  }
  return true;
}

void CustomMount::RemoveMountPoint(const std::string &path) noexcept {
  try {
    if (std::filesystem::is_empty(path)) {
      std::filesystem::remove(path);
      logger_->info("[UnMount] Remove {} ", path);
    }
  } catch (const std::exception &ex) {
    logger_->error("[UnMount] Can't remove {}", path);
  }
}

void CustomMount::SetMountOptions(MountOptions &opts) const noexcept {
  if (opts.fs.empty())
    return;
  opts.mount_flags = MS_NOSUID | MS_NODEV | MS_RELATIME;
  std::string uid_gid = "uid=";
  uid_gid += std::to_string(uid_.value_or(0));
  uid_gid += ",gid=";
  uid_gid += std::to_string(gid_.value_or(0));
  if (ptr_device_->filesystem() == "iso9660") {
    opts.mount_flags = opts.mount_flags | MS_RDONLY;
    opts.mount_data += uid_gid;
    opts.mount_data += ",iocharset=utf8";
    opts.read_only = true;
  } else if (ptr_device_->filesystem() == "vfat") {
    opts.mount_data += uid_gid;
    opts.mount_data += ",fmask=0007,dmask=0007,allow_utime=0020,"
                       "codepage=866,shortname=mixed"
                       ",utf8,flush,errors=remount-ro";
  } else if (ptr_device_->filesystem() == "exfat") {
    opts.mount_data += uid_gid;
    opts.mount_data += ",fmask=0007,dmask=0007";
  } else if (ptr_device_->filesystem() == "ntfs") {
    opts.fs += "3";
    opts.mount_data += uid_gid;
    //  opts.mount_data += ",fmask=0007,dmask=0007,iocharset=utf8";
    opts.mount_data += ",iocharset=utf8";
  } else if (ptr_device_->filesystem() == "udf") {
    opts.mount_flags = opts.mount_flags | MS_RDONLY;
    opts.mount_data += uid_gid;
    opts.mount_data += ",mode=440,dmode=550,iocharset=utf8";
    opts.read_only = true;
  } else {
    logger_->info("Filesystem {}, mounting with default parameters",
                  ptr_device_->filesystem());
  }
  logger_->debug("Mount data = {}", opts.mount_data);
}

bool CustomMount::FixNtfs(const std::string &block) const noexcept {
  logger_->info("[FixNtfs] try ro remove a dirty bit");
  std::string command = "/bin/ntfsfix -d ";
  command += block;
  int result = system(command.c_str());
  if (result != 0) {
    logger_->error("Error running ntfsfix");
    return false;
  }
  logger_->info("[FixNtfs] OK");
  return true;
}

} // namespace usbmount