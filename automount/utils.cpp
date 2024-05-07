#include "utils.hpp"
#include <acl/libacl.h>
#include <cerrno>
#include <cstring>
#include <exception>
#include <filesystem>

#include <spdlog/common.h>
#include <sstream>
#include <stdexcept>
#include <sys/acl.h>
#include <sys/types.h>
#include <syslog.h>

namespace utils {

std::shared_ptr<spdlog::logger> InitLogFile(const std::string &path) noexcept {
  namespace fs = std::filesystem;
  try {
    fs::path file_path(path);
    fs::create_directories(file_path.parent_path());
    spdlog::set_level(spdlog::level::debug);
    return spdlog::basic_logger_mt("usb-automount", path);
  } catch (const std::exception &ex) {
    openlog("alterator-usb-automount", LOG_PID, LOG_USER);
    syslog(LOG_ERR, "Can't create a log file");
    return nullptr;
  }
}

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

} // namespace acl

} // namespace utils