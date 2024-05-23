#pragma once
#include "usb_udev_device.hpp"
#include <acl/libacl.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/acl.h>
#include <sys/syslog.h>

namespace utils {

/**
 * @brief Logger initialization
 *
 * @param path string path to logfile
 * @return std::shared_ptr<spdlog::logger>
 */
std::shared_ptr<spdlog::logger> InitLogFile(const std::string &path) noexcept;

namespace acl {
/**
 * @brief convert ACL to sting
 *
 * @return std::string
 */
std::string ToString(const acl_t &) noexcept;

/**
 * @brief Delete all ACL_USER and ACL_GROUP from ACL and MASK
 * @param acl
 */
void DeleteACLUserGroupMask(acl_t &acl);

} // namespace acl

} // namespace utils