#pragma once

#include "guard_rule.hpp"
#include "serializible_for_lisp.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <set>
#include <unordered_map>

#ifdef UNIT_TEST
#include "test.hpp"
#endif

namespace guard {

/************************************************************/

/**
 * @class ConfigStatus
 * @brief Status for configuration: suspiciuos udev rules,
 * and UsbGuard Status
 * */
class ConfigStatus : public SerializableForLisp<ConfigStatus> {

private:
  const std::string usb_guard_daemon_name = "usbguard.service";
  const std::string unit_dir_path = "/lib/systemd/system";
  const std::string usbguard_default_config_path =
      "/etc/usbguard/usbguard-daemon.conf";

public:
  // warning_info : filename
  std::unordered_map<std::string, std::string> udev_warnings;
  bool udev_rules_OK;
  bool guard_daemon_OK;
  bool guard_daemon_enabled;
  bool guard_daemon_active;
  std::string daemon_config_file_path;
  // filled by ParseDaemonConfig
  std::string daemon_rules_file_path;
  bool rules_files_exists;
  std::set<std::string> ipc_allowed_users;
  std::set<std::string> ipc_allowed_groups;
  std::string implicit_policy_target;

  /// @brief Constructor checks for udev rules and daemon status
  ConfigStatus() noexcept;

  /// @brief Serialize statuses
  /// @return vector of string patrs
  vecPairs SerializeForLisp() const;

  /// @brief Checks the daemon status,fills status fields
  void CheckDaemon();

  /**
   * @brief Parses usbguard rules.conf file
   *
   * @return  std::pair<std::vector<GuardRule>,uint> Parsed rules,total lines in
   * file
   */
  std::pair<std::vector<GuardRule>, uint> ParseGuardRulesFile() const noexcept;

  /**
   * @brief Overwrite rules file, recover if USBGuard fails to srart
   *
   * @param new_content New content of file
   * @param run_daemon Leave USBGuard running
   * @return true Success
   * @return false Fail
   */
  bool OverwriteRulesFile(const std::string &new_content,
                          bool run_daemon) noexcept;
  /**
   * @brief Changes USBGuard unitfile (.service) status
   *
   * @param active If true - equivalent to "systemctl start"
   * @param enabled If true - equivalent ti "systemctl enable"
   * @return true Success
   * @return false Fail
   */
  bool ChangeDaemonStatus(bool active, bool enabled) noexcept;

  /**
   * @brief Change USBGuard implicit policy
   *
   * @param block if true - apply "block" policy
   * @return true Succeded
   * @return false Failed
   */
  bool ChangeImplicitPolicy(bool block) noexcept;

  /**
   * @brief Try to run usbguard, just for a health check. Leaves a daemon in its
   * initial state if parameter is false
   * @param run_daemon true - leave daemon running
   * @return true Succeded
   * @return false Failed
   */
  bool TryToRun(bool run_daemon) noexcept;

private:
  /// @brief Return path for the  daemon .conf file
  /// @return A string path, empty string if failed
  /// @details usbguard.service is expected to be installed in
  /// /lib/systemd/system
  std::string GetDaemonConfigPath() const;

  /**
   * @brief Parses usbguard .conf file.
   * @details Looks for rules-file path.
   * Looks for allowed users and groups.
   */
  void ParseDaemonConfig();

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

/*************************************************************/
// non-friend funcions

/// @brief  inspect udev rules for suspicious files
/// @param vec just for testing purposes
/// @return map of string file:warning
std::unordered_map<std::string, std::string> InspectUdevRules(
#ifdef UNIT_TEST
    const std::vector<std::string> *vec = nullptr
#endif
);

} // namespace guard