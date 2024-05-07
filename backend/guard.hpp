#pragma once
#include "config_status.hpp"
#include "usb_device.hpp"
#include <IPCClient.hpp>
#include <USBGuard.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace guard {

/**
 * @class Guard
 * @brief Wrapper class for USBGuard
 * @warning Guard can be constructed even if daemon in not active
 */
class Guard {
public:
  /// @brief Constructor
  Guard() noexcept;

  /// @brief List current usb devices
  /// @return vector<UsbDevices>
  std::vector<UsbDevice> ListCurrentUsbDevices() noexcept;

  /**
   * @brief Allow or block device
   * @param[in] id  string, containing numerical id of usb device
   * @param[in] allow false - block, true - allow
   * @param[in] permanent  true(default) - create permanent UsbGuard rule
   */
  bool AllowOrBlockDevice(const std::string &device_id, bool allow = false,
                          bool permanent = true) noexcept;
  /**
   * @brief check configuration of UsbGuard daemon
   * @return ConfigStatus object
   */
  ConfigStatus GetConfigStatus() noexcept;

  /**
   * @brief Parse a JSON string containing changes, recieved from alterator-usb
   * web-interface
   *
   * @param msg A JSON string containing changes for alterator-usb
   * @return json object, constaining  "STATUS","rules_OK","rules_BAD"
   * @code param example
   *
   * {
   *   "preset_mode": "manual_mode",
   *   "deleted_rules": [
   *     "6",
   *     "7"
   *   ],
   *   "appended_rules": [
   *     {
   *       "table_id": "list_unsorted_rules",
   *       "tr_id": "rule_1",
   *       "target": "allow",
   *       "fields_arr": [
   *         {
   *           "raw_rule": "allow if true"
   *         }
   *       ]
   *     }
   *   ],
   *   "run_daemon": "true",
   *   "policy_type": "radio_white_list"
   * }
   * @endcode
   * @code return example
   *
   * {
   *  "STATUS": "OK",
   *  "rules_BAD": [
   *    "rule_1"
   *   ],
   *  "rules_OK": [],
   *  "rules_DELETED" :[],
   *  "ACTION": "apply"
   * }
   * @endcode
   */
  std::optional<std::string>
  ProcessJsonRulesChanges(const std::string &msg, bool apply_changes) noexcept;

private:
  /// True if daemon is active
  bool HealthStatus() const noexcept;
  /// try to connect the UsbGuardDaemon
  void ConnectToUsbGuard() noexcept;

  const std::string kDefaultQuery = "match";
  std::unique_ptr<usbguard::IPCClient> ptr_ipc_;

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard