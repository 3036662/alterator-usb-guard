#include "guard.hpp"
#include "base64_rfc4648.hpp"
#include "config_status.hpp"
#include "csv_rule.hpp"
#include "guard_rule.hpp"
#include "guard_utils.hpp"
#include "log.hpp"
#include "rapidcsv.h"
#include "usb_device.hpp"
#include "utils.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace guard {

using ::guard::utils::Log;

Guard::Guard() noexcept : ptr_ipc_(nullptr) { ConnectToUsbGuard(); }

bool Guard::HealthStatus() const noexcept {
  return ptr_ipc_ && ptr_ipc_->isConnected();
}

std::vector<UsbDevice> Guard::ListCurrentUsbDevices() noexcept {
  std::vector<UsbDevice> res;
  if (!HealthStatus())
    return res;
  // health is ok -> get all devices
  std::vector<usbguard::Rule> rules;
  try {
    rules = ptr_ipc_->listDevices(kDefaultQuery);
    for (const usbguard::Rule &rule : rules) {
      std::vector<std::string> i_types = guard::utils::FoldUsbInterfacesList(
          rule.attributeWithInterface().toRuleString());
      for (const std::string &i_type : i_types) {
        std::vector<std::string> vid_pid;
        boost::split(vid_pid, rule.getDeviceID().toString(),
                     [](const char symbol) { return symbol == ':'; });
        UsbDevice::DeviceData dev_data{
            rule.getRuleID(),
            usbguard::Rule::targetToString(rule.getTarget()),
            rule.getName(),
            !vid_pid.empty() ? vid_pid[0] : "",
            vid_pid.size() > 1 ? vid_pid[1] : "",
            rule.getViaPort(),
            rule.getWithConnectType(),
            i_type,
            rule.getSerial(),
            rule.getHash()};
        res.emplace_back(std::move(dev_data));
      }
    }
  } catch (const std::exception &ex) {
    Log::Error() << "USBGuard error";
    Log::Error() << ex.what();
    return res;
  }
  // fill all vendor names with one read of file
  std::unordered_set<std::string> vendors_to_search;
  for (const UsbDevice &usb : res) {
    vendors_to_search.insert(usb.vid());
  }
  std::unordered_map<std::string, std::string> vendors_names{
      guard::utils::MapVendorCodesToNames(vendors_to_search)};
  for (UsbDevice &usb : res) {
    if (vendors_names.count(usb.vid()) != 0) {
      usb.vendor_name(vendors_names[usb.vid()]);
    }
  }
  return res;
}

bool Guard::AllowOrBlockDevice(const std::string &device_id, bool allow,
                               bool permanent) noexcept {
  if (device_id.empty() || !HealthStatus())
    return false;
  std::optional<uint32_t> id_numeric = ::utils::StrToUint(device_id);
  if (!id_numeric)
    return false;
  usbguard::Rule::Target policy =
      allow ? usbguard::Rule::Target::Allow : usbguard::Rule::Target::Block;
  try {
    ptr_ipc_->applyDevicePolicy(*id_numeric, policy, permanent);
  } catch (const usbguard::Exception &ex) {
    Log::Error() << "Can't add rule."
                 << "May be rule conflict happened.";
    Log::Error() << ex.what();
    return false;
  }
  return true;
}

ConfigStatus Guard::GetConfigStatus() noexcept {
  ConfigStatus config_status;
  //  TODO if daemon is on active think about creating policy before enabling
  if (!HealthStatus())
    ConnectToUsbGuard();
  config_status.guard_daemon_active = HealthStatus();
  return config_status;
}

bool Guard::DeleteRules(const std::vector<uint> &rule_indexes,
                        const Target new_policy,
                        std::vector<GuardRule> &new_rules,
                        bool &deleted_by_policy_change) noexcept {
  ConfigStatus config = GetConfigStatus();
  // make sure that old rules were successfully parsed
  std::pair<std::vector<guard::GuardRule>, uint> parsed_rules =
      config.ParseGuardRulesFile();
  if (parsed_rules.first.size() != parsed_rules.second) {
    Log::Error() << "The rules file is not completelly parsed, can't edit";
    return false;
  }
  // copy old rules,except listed in rule_indexes
  std::set<uint> unique_indexes(rule_indexes.cbegin(), rule_indexes.cend());
  for (const auto &rule : parsed_rules.first) {
    if (unique_indexes.count(rule.number()) == 0) {
      // skip rules with conflicting policy,place rest to new_rules
      // if implicit policy="allow" rules must be block or reject
      if ((new_policy == Target::allow && (rule.target() == Target::block ||
                                           rule.target() == Target::reject)) ||
          (new_policy == Target::block && rule.target() == Target::allow)) {
        new_rules.push_back(rule);
      } else {
        deleted_by_policy_change = true;
      }
    }
  }
  return true;
}

void Guard::ConnectToUsbGuard() noexcept {
  try {
    ptr_ipc_ = std::make_unique<usbguard::IPCClient>(true);
  } catch (usbguard::Exception &e) {
    Log::Warning() << "Error connecting to USBGuard daemon.";
    Log::Warning() << e.what();
  }
}

std::optional<std::string>
Guard::ApplyJsonRulesChanges(const std::string &msg) noexcept {
  namespace json = boost::json;
  // parse the json and process
  json::value json_value;
  json::object *ptr_jobj = nullptr;
  try {
    json_value = json::parse(msg);
    ptr_jobj = json_value.if_object();
    if (ptr_jobj == nullptr)
      throw std::logic_error("Can't parse JSON");
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse JSON";
    Log::Error() << ex.what();
    return std::nullopt;
  }
  // daemon target state (active or stopped)
  std::optional<bool> daemon_activate =
      guard::utils::ExtractDaemonTargetState(ptr_jobj);
  if (!daemon_activate.has_value())
    return std::nullopt;
  // policy type
  std::optional<Target> policy = guard::utils::ExtractTargetPolicy(ptr_jobj);
  if (!policy.has_value())
    return std::nullopt;
  // preset mode
  std::optional<std::string> preset_mode =
      guard::utils::ExtractPresetMode(ptr_jobj);
  if (!preset_mode.has_value())
    return std::nullopt;
  // new rules will be stored here
  bool rules_changed_by_policy{false};
  std::vector<guard::GuardRule> new_rules;
  std::vector<uint> rules_to_delete;
  std::vector<GuardRule> rules_to_add;
  ConfigStatus config_status = GetConfigStatus();
  // result json
  boost::json::object obj_result;
  // manual mode
  if (*preset_mode == "manual_mode") {
    obj_result = guard::utils::ProcessJsonManualMode(ptr_jobj, rules_to_delete,
                                                     rules_to_add);
    // if some rules are bad - just return a validation result
    if (!obj_result.at("rules_BAD").as_array().empty()) {
      return json::serialize(obj_result);
    }
    // build a vector with new rules
    if (!DeleteRules(rules_to_delete, *policy, new_rules,
                     rules_changed_by_policy))
      return std::nullopt;
    // change policy (bool) - true means block
    config_status.ChangeImplicitPolicy(*policy == Target::block);
  }
  // block all except connected
  if (*preset_mode == "put_connected_to_white_list" ||
      *preset_mode == "put_connected_to_white_list_plus_HID") {
    obj_result = ProcessJsonAllowConnected(rules_to_add);
  }
  // add HID devices to white list
  if (*preset_mode == "put_connected_to_white_list_plus_HID")
    guard::utils::AddAllowHid(rules_to_add);
  // block only usb mass storage  and MTP PTP
  if (*preset_mode == "put_disks_and_mtp_to_black") {
    guard::utils::AddBlockUsbStorages(rules_to_add);
    config_status.ChangeImplicitPolicy(false);
    obj_result["STATUS"] = "OK";
  }
  // block known android devices
  if (*preset_mode == "block_and_reject_android") {
    if (!guard::utils::AddRejectAndroid(rules_to_add))
      return std::nullopt;
    config_status.ChangeImplicitPolicy(false);
    obj_result["STATUS"] = "OK";
  }
  // add rules_to_add to new rules vector
  for (auto &rule : rules_to_add)
    new_rules.push_back(std::move(rule));
  // build one string for file overwrite
  std::string str_new_rules;
  for (const auto &rule : new_rules) {
    str_new_rules += rule.BuildString(true, true);
    str_new_rules += "\n";
  }
  // overwrite the rules and test launch if some rules were added or deleted
  if (rules_changed_by_policy || !rules_to_delete.empty() ||
      !rules_to_add.empty()) {
    if (!config_status.OverwriteRulesFile(str_new_rules, *daemon_activate)) {
      Log::Error() << "Can't launch the daemon with new rules";
      return std::nullopt;
    }
  }
  if (!config_status.ChangeDaemonStatus(*daemon_activate, *daemon_activate))
    Log::Error() << "Change the daemon status FAILED";
  return json::serialize(obj_result);
}

boost::json::object Guard::ProcessJsonAllowConnected(
    std::vector<GuardRule> &rules_to_add) noexcept {
  namespace json = boost::json;
  json::object res;
  auto config = GetConfigStatus();
  // temporary change the policy to "allow all"
  // The purpose is to launch USBGuard without blocking anything
  // to receive a list of devices
  if (!config.ChangeImplicitPolicy(false)) {
    Log::Error() << "Can't change usbguard policy";
    res["STATUS"] = "error";
    res["ERROR_MSG"] = "Failed to change usbguard policy";
    return res;
  }
  Log::Info() << "USBguard implicit policy was changed to allow all";
  // make sure that USBGuard is running
  ConnectToUsbGuard();
  if (!HealthStatus()) {
    config.TryToRun(true);
    ConnectToUsbGuard();
    if (!HealthStatus()) {
      Log::Error() << "Can't launch usbguard";
      res["STATUS"] = "error";
      res["ERROR_MSG"] = "Failed to create policy.Can't launch usbguard";
      return res;
    }
  }
  // get list of devices
  std::vector<UsbDevice> devs = ListCurrentUsbDevices();
  for (const UsbDevice &dev : devs) {
    std::stringstream string_builder;
    string_builder << "allow name " << ::utils::QuoteIfNotQuoted(dev.name())
                   << " hash " << ::utils::QuoteIfNotQuoted(dev.hash());
    try {
      rules_to_add.emplace_back(string_builder.str());
    } catch (const std::logic_error &ex) {
      Log::Error() << "Can't create a rule for device"
                   << "allow name " << ::utils::QuoteIfNotQuoted(dev.name())
                   << " hash " << ::utils::QuoteIfNotQuoted(dev.hash());
      Log::Error() << ex.what();
      res["STATUS"] = "error";
      res["ERROR_MSG"] = "Failed to create policy.Failed";
      return res;
    }
  }

  if (config.implicit_policy_target != "block") {
    Log::Info() << "Changing USBGuard policy to block all";
    config.ChangeImplicitPolicy(true);
  }
  res["STATUS"] = "OK";
  return res;
}

std::optional<std::vector<GuardRule>>
Guard::UploadRulesCsv(const std::string &file) noexcept {
  const size_t kColumnsRequired = 2;
  HealthStatus();
  try {
    std::string csv_string =
        cppcodec::base64_rfc4648::decode<std::string>(file);
    csv_string = cppcodec::base64_rfc4648::decode<std::string>(csv_string);
    // Log::Debug() << "CSV WIDE = " << csv_string;
    csv_string = ::utils::UnUtf8(csv_string);
    // Log::Debug() << "CSV (UnUtf8) = " << csv_string;
    std::stringstream sstream(csv_string);
    rapidcsv::Document doc(sstream, rapidcsv::LabelParams(-1, -1),
                           rapidcsv::SeparatorParams(','));
    if (doc.GetRowCount() == 0 || doc.GetColumnCount() < kColumnsRequired) {
      Log::Error() << "Bad csv file";
      return std::nullopt;
    }
    // Log::Debug() << csv_string;
    size_t n_rows = doc.GetRowCount();
    Log::Debug() << "Rows count" << n_rows;
    std::vector<size_t> failed_rules;
    std::vector<GuardRule> res;
    for (size_t i = 0; i < n_rows; ++i) {
      try {
        std::string raw_str_tule = utils::csv::CsvRule(doc, i).BuildString();
        // Log::Debug() << raw_str_tule;
        GuardRule tmpRule(raw_str_tule);
        // Log::Debug() << "GUARD RULE "<< tmpRule.BuildString();
        tmpRule.number(i);
        // raw rules are not supported for csv
        if (tmpRule.level() != StrictnessLevel::non_strict)
          res.emplace_back(std::move(tmpRule));
      } catch (const std::logic_error &ex) {
        Log::Debug() << "Can't parse a csv rule " << i;
        Log::Debug() << ex.what();
        failed_rules.push_back(i);
      }
    }
    if (!failed_rules.empty()) {
      Log::Warning() << "Parsing of " << failed_rules.size() << "was failed";
      return std::nullopt;
    }
    Log::Debug() << "CSV rules number = " << res.size();
    return res;
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse a csv file";
    return std::nullopt;
  }
}

} // namespace guard
