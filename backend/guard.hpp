#pragma once
#include "usb_device.hpp"
#include <IPCClient.hpp>
#include <USBGuard.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#ifdef UNIT_TEST
#include "test.hpp"
#endif
namespace guard {

/**
 * @class ConfigStatus
 * @brief Status for con1figuration: suspiciuos udev rules,
 * and UsbGuard Status
 * */
struct ConfigStatus {
  bool udev_rules_OK = false;
  std::unordered_map<std::string, std::string>
      udev_warnings; /// warning:filename
  bool guard_daemon_OK = false;
};

/**
 * @class Guard
 * @brief Wrapper class for USBGuard
 * @warning Guard can be constructed even if daemon in not active
 */
class Guard {
public:
  /// @brief Constructor
  Guard();

  /**
   * @brief List current usb devices
   * @return vector<UsbDevices>
   */
  std::vector<UsbDevice> ListCurrentUsbDevices();
  /**
   * @brief Allow or block device
   * @param[in] id  string, containing numerical id of usb device
   * @param[in] allow false - block, true - allow
   * @param[in] permanent  true(default) - create permanent UsbGuard rule
   */
  bool AllowOrBlockDevice(std::string id, bool allow = false,
                          bool permanent = true);
  /**
   * @brief check configuration of UsbGuard daemon
   */
  ConfigStatus GetConfigStatus();

private:
  const std::string default_query = "match";
  std::unique_ptr<usbguard::IPCClient> ptr_ipc;

  /// True if daemon is active
  bool HealthStatus() const;

  /// @brief  inspect udev rules for suspicious files
  /// @param vec just for testing purposes
  /// @return map of string warning : file
  std::unordered_map<std::string, std::string> InspectUdevRules(
#ifdef UNIT_TEST
      const std::vector<std::string> *vec = nullptr
#endif
  );

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard