#pragma once
#include "custom_mount.hpp"
#include "dal/dto.hpp"
#include "usb_udev_device.hpp"
#include <acl/libacl.h>
#include <chrono>
#include <cstdint>
#include <libudev.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <sys/acl.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace usbmount::utils {

using logger_t = std::shared_ptr<spdlog::logger>;

/**
 * @brief Logger initialization
 *
 * @param path string path to logfile
 * @return std::shared_ptr<spdlog::logger>
 */
std::shared_ptr<spdlog::logger> InitLogFile(const std::string &path) noexcept;

/**
 * @brief Mount or unmount device (depends on UsbUdevDevice action value )
 * @param ptr_device Device to process
 * @param logger
 */
void MountDevice(std::shared_ptr<UsbUdevDevice> ptr_device,
                 const std::shared_ptr<spdlog::logger> &logger) noexcept;

std::unordered_set<std::string>
GetSystemMountPoints(const std::shared_ptr<spdlog::logger> &logger) noexcept;

std::unordered_set<std::string>
GetSystemMountedDevices(const std::shared_ptr<spdlog::logger> &logger) noexcept;

/**
 * @brief Check for expired mountpoints in local table
 */
bool ReviewMountPoints(const std::shared_ptr<spdlog::logger> &logger) noexcept;

/**
 * @brief read /etc/login.defs for UID_MIN,UID_MAX,GID_MIN,GID_MAX
 *
 */
struct IdMinMax {
  uid_t uid_min = 0;
  uid_t uid_max = 0;
  gid_t gid_min = 0;
  gid_t gid_max = 0;
};

/**
 * @brief Get the System User ID Min Max
 * @return std::unordered_map<std::string,std::pair<uid_t,uid_t>>
 */
std::optional<IdMinMax> GetSystemUidMinMax(const logger_t &) noexcept;

/**
 * @brief Get the Possible Shells for user
 * @return std::unordered_set<std::string>
 */
std::unordered_set<std::string> GetPossibleShells(const logger_t &) noexcept;

/**
 * @brief Get the Human Users array
 * @return std::vector<dal::User>
 */
std::vector<dal::User> GetHumanUsers(const IdMinMax &,
                                     const logger_t &) noexcept;

/**
 * @brief Get the Guman Groups array
 * @return std::vector<dal::Group>
 */
std::vector<dal::Group> GetHumanGroups(const IdMinMax &,
                                       const logger_t &) noexcept;

/**
 * @brief string to uint_64
 *
 * @param str
 * @return uint64_t
 * @throws std::invalid_argument
 */
uint64_t StrToUint(const std::string &str);

bool ValidVid(const std::string &);

namespace udev {
/**
 * @brief A custom deleter for udev_enumerate struct
 */
void UdevEnumerateFree(udev_enumerate *) noexcept;
} // namespace udev

namespace acl {

/**
 * @brief convert ACL to sting
 * @return std::string
 */
std::string ToString(const acl_t &) noexcept;

/**
 * @brief Delete all ACL_USER and ACL_GROUP from ACL and MASK
 * @param acl
 */
void DeleteACLUserGroupMask(acl_t &acl);

void CreateUserAclEntry(acl_t &acl, uid_t uid);
void CreateGroupAclEntry(acl_t &acl, gid_t gid);

} // namespace acl

} // namespace usbmount::utils