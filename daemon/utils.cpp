#include "utils.hpp"
#include "config.hpp"
#include "custom_mount.hpp"
#include "dal/dto.hpp"
#include "dal/local_storage.hpp"
#include "spdlog/async.h"
#include "usb_udev_device.hpp"
#include <acl/libacl.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libudev.h>
#include <mntent.h>
#include <optional>
#include <spdlog/common.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/acl.h>
#include <sys/types.h>
#include <syslog.h>
#include <thread>
#include <unordered_set>
#include <vector>

namespace usbmount::utils {

std::shared_ptr<spdlog::logger> InitLogFile(const std::string &path) noexcept {
  namespace fs = std::filesystem;
  try {
    fs::path file_path(path);
    fs::create_directories(file_path.parent_path());
    // spdlog::set_level(spdlog::level::debug);
    // return spdlog::basic_logger_mt("usb-automount", path);
    return spdlog::basic_logger_mt<spdlog::async_factory>("usb-automount",
                                                          path);
  } catch (const std::exception &ex) {
    openlog("alterator-usb-automount", LOG_PID, LOG_USER);
    syslog(LOG_ERR, "Can't create a log file");
    return nullptr;
  }
}

void MountDevice(std::shared_ptr<UsbUdevDevice> ptr_device,
                 const std::shared_ptr<spdlog::logger> &logger) noexcept {
  try {
    if (ptr_device->subsystem() != "block")
      return;
    CustomMount mounter(ptr_device, logger);
    if (ptr_device->action() == Action::kAdd) {
      logger->info("Mount {} ", ptr_device->block_name());
      if (!mounter.Mount())
        logger->error("Mount failed");
    } else if (ptr_device->action() == Action::kRemove) {
      logger->info("Unmounting {}", ptr_device->block_name());
      mounter.UnMount();
    }
    logger->flush();
  } catch (const std::exception &ex) {
    std::cerr << ex.what();
  }
}

std::unordered_set<std::string>
GetSystemMountPoints(const std::shared_ptr<spdlog::logger> &logger) noexcept {
  std::unordered_set<std::string> mtab_mountpoints;
  // get all system mountpoints
  FILE *p_file = setmntent("/etc/mtab", "r");
  if (p_file == NULL) {
    logger->error("[UnMount] Error opening /etc/mtab");
    logger->flush();
    return mtab_mountpoints;
  }
  mntent *entry;
  while ((entry = getmntent(p_file)) != NULL) {
    if (boost::contains(entry->mnt_dir, CustomMount::mount_root)) {
      mtab_mountpoints.emplace(entry->mnt_dir);
    }
  }
  endmntent(p_file);
  return mtab_mountpoints;
}

std::unordered_set<std::string> GetSystemMountedDevices(
    const std::shared_ptr<spdlog::logger> &logger) noexcept {
  std::unordered_set<std::string> mtab_devs;
  // get all system mountpoints
  FILE *p_file = setmntent("/etc/mtab", "r");
  if (p_file == NULL) {
    logger->error("[UnMount] Error opening /etc/mtab");
    logger->flush();
    return mtab_devs;
  }
  mntent *entry;
  while ((entry = getmntent(p_file)) != NULL) {
    mtab_devs.emplace(entry->mnt_fsname);
  }
  endmntent(p_file);
  return mtab_devs;
}

bool ReviewMountPoints(const std::shared_ptr<spdlog::logger> &logger) noexcept {
  auto dbase = dal::LocalStorage::GetStorage();
  auto mtab_mountpoints = GetSystemMountPoints(logger);
  dbase->mount_points.RemoveExpired(mtab_mountpoints);
  // remove empty folders
  const std::string mount_folder = BASE_MOUNT_POINT;
  namespace fs = std::filesystem;
  try {
    if (fs::exists(mount_folder)) {
      logger->debug("[ReviewMountPoints] Inspect folders");
      for (const auto &entry : fs::directory_iterator(mount_folder)) {
        if (fs::is_directory(entry.path()) && !fs::is_empty(entry.path())) {
          for (const auto &sub_entry : fs::directory_iterator(entry.path())) {
            if (fs::is_directory(sub_entry.path()) &&
                fs::is_empty(sub_entry.path()) &&
                mtab_mountpoints.count(sub_entry.path().string()) == 0) {
              logger->info("Removing empty {}", sub_entry.path().string());
              fs::remove(sub_entry.path());
            }
          }
        }
      }
    } else {
      fs::create_directories(mount_folder);
    }
  } catch (const std::exception &ex) {
    logger->warn("Error while inpecting folders in " + mount_folder);
    logger->warn(ex.what());
  }
  return true;
}

std::optional<IdMinMax> GetSystemUidMinMax(const logger_t &logger) noexcept {
  const std::string fname = "/etc/login.defs";
  try {
    if (!std::filesystem::exists(fname)) {
      logger->error("[GetSystemUidMinMax] file {} was not found", fname);
      return std::nullopt;
    }
  } catch (const std::exception &ex) {
    logger->error("[GetSystemUidMinMax] {}", ex.what());
    return std::nullopt;
  }
  std::ifstream defs_file(fname);
  if (!defs_file.is_open()) {
    logger->error("[GetSystemUidMinMax] can't open {}", fname);
    return std::nullopt;
  }
  IdMinMax res{0, 0, 0, 0};
  bool res_ok = false;
  try {
    boost::regex whitespace("\\s+");
    std::string line;
    while (std::getline(defs_file, line)) {
      boost::trim(line);
      if (line.empty() || boost::starts_with(line, "#"))
        continue;
      std::vector<std::string> tokens;
      boost::split_regex(tokens, line, whitespace);
      if (tokens.size() > 1) {
        if (tokens[0] == "UID_MIN")
          res.uid_min = static_cast<uid_t>(StrToUint(tokens[1]));
        else if (tokens[0] == "UID_MAX")
          res.uid_max = static_cast<uid_t>(StrToUint(tokens[1]));
        else if (tokens[0] == "GID_MIN")
          res.gid_min = static_cast<gid_t>(StrToUint(tokens[1]));
        else if (tokens[0] == "GID_MAX")
          res.gid_max = static_cast<gid_t>(StrToUint(tokens[1]));
      }
      if (res.uid_min > 0 && res.uid_max > 0 && res.gid_min > 0 &&
          res.gid_max > 0) {
        res_ok = true;
        break;
      }
    }
  } catch (const std::exception &ex) {
    logger->error("Error parsing {}", fname);
    logger->error(ex.what());
    defs_file.close();
    return std::nullopt;
  }
  defs_file.close();
  return res_ok ? std::make_optional(res) : std::nullopt;
}

std::unordered_set<std::string>
GetPossibleShells(const logger_t &logger) noexcept {
  std::unordered_set<std::string> res;
  const std::string fname = "/etc/shells";
  try {
    if (!std::filesystem::exists(fname))
      throw std::runtime_error("File not found " + fname);
  } catch (const std::exception &ex) {
    logger->error("[GetPossibleShells] {}", ex.what());
    return res;
  }
  std::ifstream file(fname);
  if (!file.is_open()) {
    logger->error("Can't open file {}", fname);
    return res;
  }
  std::string line;
  while (std::getline(file, line)) {
    boost::trim(line);
    if (line.empty())
      continue;
    res.emplace(std::move(line));
    line.clear();
  }
  file.close();
  return res;
}

std::vector<dal::User> GetHumanUsers(const IdMinMax &id_limits,
                                     const logger_t &logger) noexcept {
  std::vector<dal::User> res;
  std::unordered_set<std::string> shells = GetPossibleShells(logger);
  const std::string fpath = "/etc/passwd";
  namespace fs = std::filesystem;
  try {
    if (!fs::exists(fpath))
      throw std::runtime_error("File not found " + fpath);
  } catch (const std::exception &ex) {
    logger->error("[GetHumanUsers] {}", ex.what());
    return res;
  }
  std::ifstream file(fpath);
  if (!file.is_open()) {
    logger->error("[GetHumanUsers] Can't open file {}", fpath);
    return res;
  }
  try {
    std::string line;
    while (std::getline(file, line)) {
      std::vector<std::string> tokens;
      boost::split(tokens, line,
                   [](const char symbol) { return symbol == ':'; });
      if (tokens.size() == 7 && !tokens[0].empty() &&
          shells.count(tokens[6]) > 0) {
        try {
          uid_t uid = StrToUint(tokens[2]);
          gid_t gid = StrToUint(tokens[3]);
          if (uid >= id_limits.uid_min && uid <= id_limits.uid_max &&
              gid >= id_limits.gid_min && gid <= id_limits.gid_max)
            res.emplace_back(uid, std::move(tokens[0]));
        } catch (const std::exception &ex) {
          logger->warn("Can't parse {} string: {}", fpath, line);
          continue;
        }
      }
      tokens.clear();
    }
  } catch (const std::exception &ex) {
    logger->error("[GetHumanUsers] {} ", ex.what());
  }
  file.close();
  return res;
}

std::vector<dal::Group> GetHumanGroups(const IdMinMax &id_limits,
                                       const logger_t &logger) noexcept {
  std::vector<dal::Group> res;
  const std::string fpath = "/etc/group";
  namespace fs = std::filesystem;
  try {
    if (!fs::exists(fpath))
      throw std::runtime_error("File not found " + fpath);
  } catch (const std::exception &ex) {
    logger->error("[GetHumanUsers] {}", ex.what());
    return res;
  }
  std::ifstream file(fpath);
  if (!file.is_open()) {
    logger->error("[GetHumanUsers] Can't open file {}", fpath);
    return res;
  }
  try {
    std::string line;
    while (std::getline(file, line)) {
      std::vector<std::string> tokens;
      boost::split(tokens, line,
                   [](const char symbol) { return symbol == ':'; });
      if (tokens.size() >= 3 && !tokens[0].empty()) {
        try {
          uid_t uid = StrToUint(tokens[2]);
          if (uid >= id_limits.uid_min && uid <= id_limits.uid_max) {
            res.emplace_back(uid, std::move(tokens[0]));
          }
        } catch (const std::exception &ex) {
          logger->warn("Can't parse {} string: {}", fpath, line);
          continue;
        }
      }
      tokens.clear();
    }
  } catch (const std::exception &ex) {
    logger->error("[GetHumanGroups] {}", ex.what());
  }

  file.close();
  return res;
}

uint64_t StrToUint(const std::string &str) {
  uint64_t res = 0;
  size_t pos = 0;
  if (!str.empty() && str[0] == '-')
    throw std::invalid_argument("Can't parse to uint " + str);
  res = static_cast<uint64_t>(std::stoul(str, &pos, 10));
  if (pos != str.size())
    throw std::invalid_argument("Can't parse to uint " + str);
  return res;
}

bool ValidVid(const std::string &str) {
  if (str.size() != 4)
    return false;
  if (str[0] == '-')
    return false;
  size_t pos = 0;
  int val = std::stoi(str, &pos, 16);
  std::cerr << val << '\n';
  if (pos != str.size())
    return false;
  if (val < 0 || val > 0xFFFF)
    return false;
  return true;
}

std::string SanitizeMount(const std::string &str) noexcept {
  std::string res;
  for (const auto symbol : str) {
    if (symbol == '\\' || symbol == '/' || symbol == '\'' || symbol == '\"')
      res.push_back('_');
    else
      res.push_back(symbol);
  }
  return res;
}

namespace udev {
void UdevEnumerateFree(udev_enumerate *udev_en) noexcept {
  if (udev_en != NULL)
    udev_enumerate_unref(udev_en);
}
} // namespace udev

namespace acl {

std::string ToString(const acl_t &acl) noexcept {
  std::stringstream str;
  acl_entry_t entry;
  acl_tag_t tag;
  uint max_it = 100;
  uint it_number = 0;
  for (int entry_id = ACL_FIRST_ENTRY;; entry_id = ACL_NEXT_ENTRY) {
    ++it_number;
    if (it_number > max_it)
      break;
    if (acl_get_entry(acl, entry_id, &entry) != 1)
      break;
    if (acl_get_tag_type(entry, &tag) == -1)
      str << strerror(errno);
    switch (tag) {
    case ACL_USER_OBJ:
      str << "user_obj ";
      break;
    case ACL_USER: {
      str << " user ";
      uid_t *uidp = static_cast<uid_t *>(acl_get_qualifier(entry));
      if (uidp == NULL)
        str << "acl_get_qualifier FAILED";
      else
        str << " " << *uidp << " ";
      acl_free(uidp);
    } break;
    case ACL_GROUP_OBJ:
      str << " group_obj ";
      break;
    case ACL_GROUP: {
      str << " group ";
      uid_t *gidp = static_cast<uid_t *>(acl_get_qualifier(entry));
      if (gidp == NULL)
        str << "acl_get_qualifier FAILED";
      else
        str << " " << *gidp << " ";
      acl_free(gidp);
    } break;
    case ACL_MASK:
      str << " mask ";
      break;
    case ACL_OTHER:
      str << " other ";
      break;
    default:
      str << "UNDEFINED";
      break;
    }
    // permissions
    acl_permset_t permset;
    if (acl_get_permset(entry, &permset) == -1)
      str << "acl_get_permset FAILED";
    int permval = acl_get_perm(permset, ACL_READ);
    str << (permval == 1 ? "r" : "-");
    permval = acl_get_perm(permset, ACL_WRITE);
    str << (permval == 1 ? "w" : "-");
    permval = acl_get_perm(permset, ACL_EXECUTE);
    str << (permval == 1 ? "x" : "-");
  }
  return str.str();
}

void DeleteACLUserGroupMask(acl_t &acl) {
  uint max_it = 100;
  uint it_number = 0;
  acl_entry_t entry;
  acl_tag_t tag;
  for (int entry_id = ACL_FIRST_ENTRY;; entry_id = ACL_NEXT_ENTRY) {
    ++it_number;
    if (it_number > max_it)
      throw std::logic_error("max iterations limit exceeded");
    if (acl_get_entry(acl, entry_id, &entry) != 1)
      break;
    if (acl_get_tag_type(entry, &tag) == -1)
      throw std::logic_error(strerror(errno));
    if (tag == ACL_USER || tag == ACL_GROUP || tag == ACL_MASK)
      if (acl_delete_entry(acl, entry) == -1)
        throw std::runtime_error(strerror(errno));
  }
}

void CreateUserAclEntry(acl_t &acl, uid_t uid) {
  acl_entry_t entry;
  acl_permset_t permset;
  if (acl_create_entry(&acl, &entry) != 0)
    throw std::runtime_error("Can't create ACL entry");
  if (acl_get_permset(entry, &permset) != 0)
    throw std::runtime_error("acl_get_permset failed");
  if (acl_add_perm(permset, ACL_READ | ACL_EXECUTE) != 0)
    throw std::runtime_error("acl_add_perm failed");
  if (acl_set_tag_type(entry, ACL_USER) != 0)
    throw std::runtime_error("acl_set_tag_type failed");
  if (acl_set_qualifier(entry, &uid) != 0)
    throw std::runtime_error("acl_set_qualifier failed");
  if (acl_set_permset(entry, permset) != 0)
    throw std::runtime_error("acl_set_permset failed");
}

void CreateGroupAclEntry(acl_t &acl, gid_t gid) {
  acl_entry_t entry;
  acl_permset_t permset;
  if (acl_create_entry(&acl, &entry) != 0)
    throw std::runtime_error("Can't create ACL entry for group");
  if (acl_get_permset(entry, &permset) != 0)
    throw std::runtime_error("acl_get_permset failed");
  if (acl_add_perm(permset, ACL_READ | ACL_EXECUTE) != 0)
    throw std::runtime_error("acl_add_perm failed");
  if (acl_set_tag_type(entry, ACL_GROUP) != 0)
    throw std::runtime_error("acl_set_tag_type failed");
  if (acl_set_qualifier(entry, &gid) != 0)
    throw std::runtime_error("acl_set_qualifier failed");
  if (acl_set_permset(entry, permset) != 0)
    throw std::runtime_error("acl_set_permset failed");
}

} // namespace acl

} // namespace usbmount::utils