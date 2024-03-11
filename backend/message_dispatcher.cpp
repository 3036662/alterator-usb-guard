#include "message_dispatcher.hpp"
#include "guard_rule.hpp"
#include "guard_utils.hpp"
#include "log.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_set>
#include <vector>

using guard::utils::Log;
using namespace utils;

MessageDispatcher::MessageDispatcher(guard::Guard &guard) noexcept
    : guard_(guard) {}

bool MessageDispatcher::Dispatch(const LispMessage &msg) const noexcept {
  // list usbs
  if (msg.action == "list" && msg.objects == "list_curr_usbs") {
    return ListUsbDevices();
  }
  // allow device with id
  if (msg.action == "read" && msg.objects == "usb_allow") {
    return AllowDevice(msg);
  }
  // block device with id
  if (msg.action == "read" && msg.objects == "usb_block") {
    return BlockDevice(msg);
  }
  // get udev rules list
  if (msg.action == "list" && msg.objects == "check_config_udev") {
    return CheckConfig();
  }
  // get congiguration info
  if (msg.action == "read" && msg.objects == "config_status") {
    Log::Info() << "Get config status";
    std::cout << ToLispAssoc(guard_.GetConfigStatus());
    return true;
  }
  // list usbguard rules
  if (msg.action == "list" && msg.objects == "list_rules" &&
      msg.params.count("level") > 0) {
    return ListUsbGuardRules(msg);
  }
  // save changes rules
  if (msg.action == "read" && msg.objects == "apply_changes") {
    return SaveChangeRules(msg);
  }
  // upload rules file
  if (msg.action == "read" && msg.objects == "rules_upload") {
    return UploadRulesFile(msg);
  }
  // empty response
  std::cout << "(\n)\n";
  return true;
}

bool MessageDispatcher::UploadRulesFile(const LispMessage &msg) const noexcept {
  auto start = std::chrono::steady_clock::now();
  Log::Debug() << "Uploading file started";
  if (msg.params.count("upload_rules") == 0 ||
      msg.params.at("upload_rules").empty()) {
    std::cout << kMessBeg << kMessEnd;
    Log::Warning() << "Empty rules file";
    return true;
  }
  std::optional<std::vector<guard::GuardRule>> vec_rules =
      guard::utils::UploadRulesCsv(msg.params.at("upload_rules"));
  Log::Debug() << "Rules parsed";
  if (vec_rules.has_value() && !vec_rules->empty()) {
    std::optional<std::string> js_arr = guard::utils::BuildJsonArrayOfUpploaded(
        vec_rules.value_or(std::vector<guard::GuardRule>()));
    // Log::Debug() << js_arr.value_or("no json arr");
    vecPairs vec_result;
    if (js_arr.has_value()) {
      vec_result.emplace_back("status", "OK");
      vec_result.emplace_back("response_json",
                              EscapeQuotes(js_arr.value_or("")));
      // Log::Debug() << EscapeQuotes(js_arr.value_or("no json arr"));
    } else {
      Log::Error() << "Nothing build to JSON obj";
      vec_result.emplace_back("status", "BAD");
      vec_result.emplace_back("err_what", "0 parsed");
    }
    Log::Debug() << "Elapsed(ms)=" << since(start).count();
    std::cout << ToLispAssoc(
        SerializableForLisp<vecPairs>(std::move(vec_result)));
    return true;
  }
  Log::Error() << "Empty rules list";
  vecPairs vec_result;
  vec_result.emplace_back("status", "BAD");
  vec_result.emplace_back("err_what", "0 parsed");
  std::cout << ToLispAssoc(
      SerializableForLisp<vecPairs>(std::move(vec_result)));
  Log::Debug() << "Elapsed(ms)=" << since(start).count();
  return true;
}

bool MessageDispatcher::SaveChangeRules(const LispMessage &msg) const noexcept {
  auto start = std::chrono::steady_clock::now();
  Log::Debug() << "Time measurement has started";
  if (msg.params.count("changes_json") == 0 ||
      msg.params.find("changes_json")->second.empty()) {
    std::cout << kMessBeg << kMessEnd;
    Log::Warning() << "bad request for rules,doing nothing";
    return true;
  }
  std::string json_string = msg.params.at("changes_json");
  boost::replace_all(json_string, "\\\\\"", "\\\"");
  std::optional<std::string> result = guard_.ApplyJsonRulesChanges(json_string);
  if (result)
    *result = EscapeQuotes(*result);
  vecPairs vec_result;
  if (result) {
    vec_result.emplace_back("status", "OK");
    vec_result.emplace_back("ids_json", std::move(*result));
  } else {
    vec_result.emplace_back("status", "FAILED");
  }
  std::cout << ToLispAssoc(
      SerializableForLisp<vecPairs>(std::move(vec_result)));
  std::cout.flush();
  Log::Debug() << "[DEBUG] Elapsed(ms)=" << since(start).count();
  return true;
}

bool MessageDispatcher::ListUsbGuardRules(
    const LispMessage &msg) const noexcept {
  guard::StrictnessLevel level =
      guard::GuardRule::StrToStrictnessLevel(msg.params.at("level"));
  auto start = std::chrono::steady_clock::now();
  Log::Debug() << "Time measurement has started";
  std::vector<guard::GuardRule> vec_rules =
      guard_.GetConfigStatus().ParseGuardRulesFile().first;
  std::cout << kMessBeg;
  std::string response;
  // map vendor ids to strings
  if (level == guard::StrictnessLevel::vid_pid) {
    std::unordered_set<std::string> vendors;
    for (const auto &rule : vec_rules) {
      if (rule.vid().has_value())
        vendors.insert(rule.vid().value_or(""));
    }
    auto vendors_names = guard::utils::MapVendorCodesToNames(vendors);
    for (auto &rule : vec_rules) {
      if (rule.vid().has_value() &&
          vendors_names.count(rule.vid().value_or("")) > 0) {
        rule.vendor_name(vendors_names.at(rule.vid().value_or("")));
      }
    }
  }
  for (const auto &rule : vec_rules) {
    if (rule.level() == level) {
      response += ToLisp(rule);
    }
  }
  std::cout << response << kMessEnd;
  Log::Debug() << "Elapsed(ms)=" << since(start).count();
  return true;
}

bool MessageDispatcher::ListUsbDevices() const noexcept {
  Log::Debug() << "Time measurement has started";
  std::vector<guard::UsbDevice> vec_usb = guard_.ListCurrentUsbDevices();
  std::cout << kMessBeg;
  for (const auto &usb : vec_usb) {
    std::cout << ToLisp(usb);
  }
  std::cout << kMessEnd;
  return true;
}

bool MessageDispatcher::AllowDevice(const LispMessage &msg) const noexcept {
  if (msg.params.count("usb_id") == 0 ||
      msg.params.find("usb_id")->second.empty()) {
    std::cout << kMessBeg << kMessEnd;
    Log::Warning() << "Bad request for usb allow,doing nothing";
    return true;
  }
  if (guard_.AllowOrBlockDevice(msg.params.find("usb_id")->second, true)) {
    std::cout << kMessBeg << "status" << WrapWithQuotes("OK") << kMessEnd;
  } else {
    std::cout << kMessBeg << "status" << WrapWithQuotes("FAIL") << kMessEnd;
  }
  return true;
}

bool MessageDispatcher::BlockDevice(const LispMessage &msg) const noexcept {
  if (msg.params.count("usb_id") == 0 ||
      msg.params.find("usb_id")->second.empty()) {
    std::cout << kMessBeg << kMessEnd;
    Log::Warning() << "Bad request for usb allow,doing nothing";
    return true;
  }
  if (guard_.AllowOrBlockDevice(msg.params.find("usb_id")->second, false)) {
    std::cout << kMessBeg << "status" << WrapWithQuotes("OK") << kMessEnd;
  } else {
    std::cout << kMessBeg << "status" << WrapWithQuotes("FAIL") << kMessEnd;
  }
  return true;
}

bool MessageDispatcher::CheckConfig() const noexcept {
  Log::Info() << "Check config";
  std::string str = kMessBeg;
  for (const auto &pair : guard_.GetConfigStatus().udev_warnings) {
    str += ToLisp({"label_udev_rules_filename", pair.first});
  }
  str += kMessEnd;
  std::cout << str;
  return true;
}