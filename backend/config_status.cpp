#include "config_status.hpp"
#include "guard_utils.hpp"
#include "log.hpp"
#include "systemd_dbus.hpp"
#include "utils.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace guard {
using guard::utils::Log;
using ::utils::FindAllFilesInDirRecursive;

ConfigStatus::ConfigStatus() noexcept
    : udev_warnings{utils::InspectUdevRules()},
      udev_rules_OK{udev_warnings.empty()}, guard_daemon_OK{false},
      guard_daemon_enabled{false}, guard_daemon_active{false},
      daemon_config_file_path{GetDaemonConfigPath()} {
  CheckDaemon();
  ParseDaemonConfig();
}

vecPairs ConfigStatus::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("udev", udev_rules_OK ? "OK" : "BAD");
  res.emplace_back("usbguard", guard_daemon_OK ? "OK" : "BAD");
  res.emplace_back("usbguard_active",
                   guard_daemon_active ? "ACTIVE" : "STOPPED");
  res.emplace_back("usbguard_enabled",
                   guard_daemon_enabled ? "ENABLED" : "DISABLED");
  res.emplace_back("rules_file_exists", rules_files_exists ? "TRUE" : "FALSE");
  res.emplace_back("allowed_users", boost::join(ipc_allowed_users, ", "));
  res.emplace_back("allowed_groups", boost::join(ipc_allowed_groups, ", "));
  res.emplace_back("implicit_policy", implicit_policy_target);
  return res;
}

void ConfigStatus::CheckDaemon() noexcept {
  dbus_bindings::Systemd systemd;
  std::optional<bool> enabled = systemd.IsUnitEnabled(usb_guard_daemon_name);
  std::optional<bool> active = systemd.IsUnitActive(usb_guard_daemon_name);
  if (enabled.has_value())
    guard_daemon_enabled = enabled.value_or(false);
  else
    Log::Error() << "Can't check if usbguard service is enabled";
  if (active.has_value())
    guard_daemon_active = active.value_or(false);
  else
    Log::Error() << "Can't check if usbguard service is active";
  guard_daemon_OK = guard_daemon_enabled && guard_daemon_active;
}

std::string ConfigStatus::GetDaemonConfigPath() const noexcept {
  std::string res;
  const std::string full_path_to_unit =
      unit_dir_path + "/" + usb_guard_daemon_name;
  try {
    // open unit (.service file)
    if (std::filesystem::exists(full_path_to_unit)) {
      std::ifstream file_unit(full_path_to_unit);
      if (file_unit.is_open()) {
        std::string line;
        // find ExecStart string
        while (getline(file_unit, line)) {
          std::optional<std::string> val = ExctractConfigFileName(line);
          if (val.has_value()) {
            res = val.value();
            break;
          }
          line.clear();
        }
        file_unit.close();
      } else {
        Log::Error() << "Can't read " << full_path_to_unit;
      }
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't find usbguard config file";
    Log::Error() << ex.what();
  }
  return res;
}

std::optional<std::string>
ConfigStatus::ExctractConfigFileName(const std::string &line) const noexcept {
  using utils::Log;
  std::string res;
  if (!boost::algorithm::contains(line, "ExecStart"))
    return std::nullopt;
  std::vector<std::string> tokens;
  // split by space, find next after -c
  try {
    boost::split(tokens, line,
                 [](const char symbol) noexcept { return symbol == ' '; });
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
  }
  auto it_token = std::find(tokens.begin(), tokens.end(), "-c");
  if (it_token != tokens.end())
    ++it_token;
  if (it_token != tokens.end() && !it_token->empty()) {
    *it_token = boost::trim_copy(*it_token); // strong ex guarantee
    try {
      if (std::filesystem::exists(*it_token)) {
        res = std::move(*it_token);
        Log::Info() << "Found config file for usbguard " << res;
      } else {
        Log::Warning() << "Can't find path to .conf file in .service file";

        if (std::filesystem::exists(usbguard_default_config_path)) {
          Log::Warning() << "Default config found "
                         << usbguard_default_config_path;
          res = usbguard_default_config_path;
        }
      }
    } catch (const std::exception &ex) {
      Log::Error() << "Filesystem error";
      Log::Error() << ex.what();
    }
  }
  return res;
}

bool ConfigStatus::ExtractRuleFilePath(const std::string &line) noexcept {
  if (boost::starts_with(line, "RuleFile=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string path_to_rules(line, pos);
      boost::trim(path_to_rules);
      Log::Info() << "Find path to rules file " << path_to_rules;
      if (!path_to_rules.empty()) {
        daemon_rules_file_path = std::move(path_to_rules);
        try {
          rules_files_exists = std::filesystem::exists(daemon_rules_file_path);
        } catch (const std::exception &ex) {
          Log::Error() << "Can't check if rules file " << daemon_rules_file_path
                       << " exists";
          return false;
        }
        return true;
      }
    }
  }
  return false;
}

bool ConfigStatus::ExctractUsers(const std::string &line) noexcept {
  if (boost::starts_with(line, "IPCAllowedUsers=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string users_string(line, pos);
      users_string = boost::trim_copy(users_string);
      Log::Info() << "Found users in conf file " << line;
      // split by space and add to set
      std::vector<std::string> splitted_string;
      try {
        boost::split(splitted_string, users_string,
                     [](const char symbol) { return symbol == ' '; });
      } catch (const std::exception &ex) {
        Log::Error() << ex.what();
      }
      for (std::string &str : splitted_string) {
        boost::trim(str);
        if (!str.empty()) {
          ipc_allowed_users.insert(str);
        }
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::CheckUserFiles(const std::string &line) noexcept {
  if (boost::starts_with(line, "IPCAccessControlFiles=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string path_to_folder(line, pos);
      boost::trim(path_to_folder);
      Log::Info() << "Found users control folder " << line;
      try {
        std::filesystem::path fs_path_to_folder(path_to_folder);
        std::vector<std::string> files =
            FindAllFilesInDirRecursive({path_to_folder, ""});
        for (std::string &str : files) {
          boost::trim(str);
          if (!str.empty()) {
            ipc_allowed_users.insert(
                std::filesystem::path(str).filename().string());
          }
        }
      } catch (const std::exception &ex) {
        Log::Error() << "Can't check users control folder" << path_to_folder;
        Log::Error() << ex.what();
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::ExtractGroups(const std::string &line) noexcept {
  if (boost::starts_with(line, "IPCAllowedGroups=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::vector<std::string> splitted_string;
      std::string groups_string(line, pos);
      try {
        boost::split(splitted_string, groups_string,
                     [](const char symbol) { return symbol == ' '; });
      } catch (const std::exception &ex) {
        Log::Error() << ex.what();
      }
      for (std::string &str : splitted_string) {
        boost::trim(str);
        if (!str.empty()) {
          ipc_allowed_groups.insert(str);
        }
      }
    }
    return true;
  }
  return false;
}

bool ConfigStatus::ExtractPolicy(const std::string &line) noexcept {
  if (boost::starts_with(line, "ImplicitPolicyTarget=")) {
    size_t pos = line.find('=');
    if (pos != std::string::npos && ++pos < line.size()) {
      std::string implicit_target(line, pos);
      boost::trim(implicit_target);
      if (!implicit_target.empty()) {
        implicit_policy_target = std::move(implicit_target);
        Log::Info() << "Implicit policy target = " << implicit_policy_target;
      }
    }
    return true;
  }
  return false;
}

void ConfigStatus::ParseDaemonConfig() noexcept {
  // cleanup
  ipc_allowed_users.clear();
  ipc_allowed_groups.clear();
  daemon_rules_file_path.clear();
  rules_files_exists = false;
  if (daemon_config_file_path.empty())
    return;
  // open config
  std::ifstream file_dconfig(daemon_config_file_path);
  if (!file_dconfig.is_open()) {
    Log::Error() << "Can't open daemon config file " << daemon_config_file_path;
    return;
  }
  // parse
  std::string line;
  while (getline(file_dconfig, line)) {
    boost::trim(line);
    // rule file path
    if (ExtractRuleFilePath(line))
      continue;
    // parse allowed user from conf file
    if (ExctractUsers(line))
      continue;
    // parse allowed users folder and find all files in folder
    if (CheckUserFiles(line))
      continue;
    // find all groups in config file
    if (ExtractGroups(line))
      continue;
    // ImplicitPolicyTarget
    ExtractPolicy(line);
  }
  file_dconfig.close();
}

std::pair<std::vector<GuardRule>, uint>
ConfigStatus::ParseGuardRulesFile() const noexcept {
  std::pair<std::vector<GuardRule>, uint> res;
  res.second = 0;
  try {
    if (!std::filesystem::exists(daemon_rules_file_path)) {
      Log::Warning() << "The rules file for usbguard doesn't exist.";
      return res;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse rules file " << daemon_rules_file_path;
    return res;
  }
  std::ifstream file(daemon_rules_file_path);
  if (!file.is_open()) {
    Log::Error() << "Can't open file " << daemon_rules_file_path;
    return res;
  }
  // Parse the file
  std::string line;
  uint counter = 0;
  uint counter_fails = 0;
  while (std::getline(file, line)) {
    try {
      res.first.emplace_back(GuardRule(line));
      res.first.back().number(counter);
      ++counter;
    } catch (const std::logic_error &ex) {
      Log::Error() << "Can't parse the rule " << line;
      ++counter_fails;
    }
    line.clear();
  }
  file.close();

  res.second = counter + counter_fails;
  Log::Info() << "Parsed " << res.first.size() << " rules."
              << " Failed " << counter_fails;
  return res;
}

/***********************************************************/

bool ConfigStatus::OverwriteRulesFile(const std::string &new_content,
                                      bool run_daemon) noexcept {
  // temporary change the policy to "allow all"
  // The purpose is to launch USBGuard without blocking anything
  // to make sure it can parse new rules
  bool initial_policy = implicit_policy_target == "block";
  if (!ChangeImplicitPolicy(false)) {
    Log::Error() << "Can't change usbguard policy";
    return false;
  }
  if (rules_files_exists) {
    // read old rules
    std::ifstream old_file(daemon_rules_file_path);
    if (!old_file.is_open()) {
      Log::Error() << "Can't open file " << daemon_rules_file_path;
      return false;
    }
    std::stringstream old_content;
    old_content << old_file.rdbuf();
    old_file.close();

    // write new rules
    std::ofstream file(daemon_rules_file_path);
    if (!file.is_open()) {
      Log::Error() << "Can't open file " << daemon_rules_file_path
                   << " for writing";
      return false;
    }
    file << new_content;
    file.close();
    // parse new rules
    auto parsed_new_file = ParseGuardRulesFile();
    // If some errors occurred while parsing new rules - recover
    if (parsed_new_file.first.size() != parsed_new_file.second ||
        !TryToRun(run_daemon)) {
      Log::Error() << "Error parsing new rules,old file will be recovered";
      std::ofstream file3(daemon_rules_file_path);
      if (!file3.is_open()) {
        Log::Error() << "[ERROR] Can't open file " << daemon_rules_file_path
                     << " for writing";
        return false;
      }
      file3 << old_content.str();
      file3.close();
      dbus_bindings::Systemd sysd;
      // try to run with recovered rules
      if (!TryToRun(run_daemon)) {
        Log::Error()
            << "Can't start usbguard service. UsbGuard will be disabled";
        ChangeImplicitPolicy(false);
        TryToRun(false);
        ChangeDaemonStatus(false, false);
      }
      return false;
    }
  }

  // restore the policy
  if (!ChangeImplicitPolicy(initial_policy)) {
    Log::Error() << "Can't change usbguard policy";
    return false;
  }
  TryToRun(run_daemon);
  return true;
}

bool ConfigStatus::TryToRun(bool run_daemon) const noexcept {
  dbus_bindings::Systemd sysd;
  auto init_state = sysd.IsUnitActive(usb_guard_daemon_name);
  if (!init_state.has_value())
    return false;
  Log::Info() << "Usbguard is "
              << (init_state.value_or("") ? "active" : "inactive");
  // if stopped - try to start
  if (!init_state.value_or(false)) {
    auto result = sysd.StartUnit(usb_guard_daemon_name);
    Log::Info() << "Test run - "
                << ((result.has_value() && *result) ? "OK" : "FAIL");
    if (!run_daemon) {
      sysd.StopUnit(usb_guard_daemon_name);
    }
    return result.value_or(false);
  }
  // if daemon is active - restart
  auto result = sysd.RestartUnit(usb_guard_daemon_name);
  Log::Info() << "Restart - " << (result.value_or(false) ? "OK" : "FAIL");
  if (init_state.has_value() && !init_state.value_or(false) && !run_daemon) {
    sysd.StopUnit(usb_guard_daemon_name);
  }
  return result.value_or(false);
}

bool ConfigStatus::ChangeDaemonStatus(bool active,
                                      bool enabled) const noexcept {
  dbus_bindings::Systemd sysd;
  auto init_state = sysd.IsUnitActive(usb_guard_daemon_name);
  if (!init_state.has_value())
    return false;
  Log::Info() << "Usbguard is " << (*init_state ? "active" : "inactive");
  auto enabled_state = sysd.IsUnitEnabled(usb_guard_daemon_name);
  if (!enabled_state) {
    Log::Error() << "Can't define if USBGuard enabled or disabled";
    return false;
  }
  Log::Info() << "Usbguard is " << (*enabled_state ? "enabled" : "disabled");
  // if we need to stop the service
  if (*init_state && !active) {
    Log::Info() << "Stopping the service";
    auto res = sysd.StopUnit(usb_guard_daemon_name);
    if (!res || !*res) {
      Log::Error() << "Can't stop the USBGuard";
      return false;
    }
  } else if (!*init_state && active) {
    Log::Info() << "Starting the service";
    auto res = sysd.StartUnit(usb_guard_daemon_name);
    if (!res || !*res) {
      Log::Error() << "Can't start the USBGuard";
      return false;
    }
  }
  // if now the daemon is enabled and we need to disable it
  if (*enabled_state && !enabled) {
    Log::Info() << "Disabling the service";
    auto res = sysd.DisableUnit(usb_guard_daemon_name);
    if (!res || !*res) {
      Log::Error() << "Can't disable the USBGuard";
      return false;
    }
  } else if (!*enabled_state && enabled) {
    auto res = sysd.EnableUnit(usb_guard_daemon_name);
    Log::Info() << "Enabling the service";
    if (!res || !*res) {
      Log::Error() << "Can't enable the USBGuard";
      return false;
    }
  }
  return true;
}

bool ConfigStatus::ChangeImplicitPolicy(bool block) noexcept {
  if (block && implicit_policy_target == "block")
    return true;
  if (!block && implicit_policy_target == "allow")
    return true;
  Log::Debug() << "Changing implicit policy to " << (block ? "block" : "allow");
  try {
    if (!std::filesystem::exists(daemon_config_file_path)) {
      Log::Error() << "Config file " << daemon_config_file_path
                   << "doesn't exist.";
      return false;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Error looking for config file at path "
                 << daemon_config_file_path;
    Log::Error() << ex.what();
    return false;
  }
  std::string new_target = block ? "block" : "allow";
  // open config
  std::ifstream file_config(daemon_config_file_path);
  if (!file_config.is_open()) {
    Log::Error() << "Can't open daemon config file " << daemon_config_file_path;
    return false;
  }
  std::stringstream new_content;
  std::stringstream old_content;
  std::string line;
  while (getline(file_config, line)) {
    old_content << line << "\n";
    boost::trim(line);
    if (boost::starts_with(line, "ImplicitPolicyTarget=")) {
      size_t pos = line.find('=');
      if (pos != std::string::npos && ++pos < line.size()) {
        line.erase(pos, line.size() - pos);
        line += new_target;
      }
    }
    new_content << line << "\n";
    line.clear();
  }
  file_config.close();
  // write to config
  std::ofstream f_out(daemon_config_file_path);
  if (!f_out.is_open()) {
    Log::Error() << "Can't open file " << daemon_config_file_path
                 << " for writing";
    return false;
  }
  f_out << new_content.str();
  f_out.close();
  ParseDaemonConfig();
  return true;
}

} // namespace guard