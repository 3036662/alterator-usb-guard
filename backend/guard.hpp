#pragma once
#include "config_status.hpp"
#include "guard_rule.hpp"
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
   *  "rules_OK": []
   * }
   * @endcode
   */
  std::optional<std::string>
  ApplyJsonRulesChanges(const std::string &msg) noexcept;

  std::optional<std::vector<GuardRule>>
  UploadRules(const std::string &file) noexcept;

private:
  /// True if daemon is active
  bool HealthStatus() const noexcept;
  /// try to connect the UsbGuardDaemon
  void ConnectToUsbGuard() noexcept;

  /**
   * @brief Reads rules from USB Guard config,
   * deletes by index, and skips rules, conflicting with a new policy
   * @param rule_indexes Order numbers of rules to delete
   * @param new_policy New implicit policy (allow || block)
   * @param[out] new_rules Vector, where the result will be appended
   * @param[out] deleted_by_policy_change If function will skip some
   * rules via conflict with new policy, this flag will be set to true
   * @return true on success
   */
  bool DeleteRules(const std::vector<uint> &rule_indexes, Target new_policy,
                   std::vector<GuardRule> &new_rules,
                   bool &deleted_by_policy_change) noexcept;

  /**
   * @brief Utility method to add all connected devices to allow (white) list
   * @param[out] rules_to_add vector,  where new rules will be appended
   * @return boost::json::object ["STATUS":"OK"]
   */
  boost::json::object
  ProcessJsonAllowConnected(std::vector<GuardRule> &rules_to_add) noexcept;

  const std::string kDefaultQuery = "match";
  std::unique_ptr<usbguard::IPCClient> ptr_ipc_;

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard