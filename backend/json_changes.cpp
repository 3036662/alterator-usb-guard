#include "json_changes.hpp"
#include "common_utils.hpp"
#include "guard_rule.hpp"
#include "json_rule.hpp"
#include "log.hpp"
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace guard::json {

using common_utils::Log;

JsonChanges::JsonChanges(const std::string &msg)
    : p_jobj_(nullptr), daemon_activate_(false), target_policy_(Target::block),
      rules_changed_by_policy_(false) {
  std::logic_error common_ex("Can't parse JSON");
  try {
    json_value_ = json::parse(msg);
    p_jobj_ = json_value_.if_object();
    if (p_jobj_ == nullptr) {
      throw common_ex;
    }
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
    throw ex;
  }
  ExtractDaemonTargetState();
  ExtractTargetPolicy();
  ExtractPresetMode();
}

void JsonChanges::ExtractDaemonTargetState() {
  if (p_jobj_ != nullptr && p_jobj_->contains("run_daemon") &&
      p_jobj_->at("run_daemon").is_string()) {
    daemon_activate_ = p_jobj_->at("run_daemon").as_string() == "true";
    return;
  }
  throw std::runtime_error("No target daemon state is found in JSON");
}

void JsonChanges::ExtractTargetPolicy() {
  if (p_jobj_ != nullptr && p_jobj_->contains("policy_type") &&
      p_jobj_->at("policy_type").is_string()) {
    target_policy_ =
        p_jobj_->at("policy_type").as_string() == "radio_white_list"
            ? Target::block
            : Target::allow;
    return;
  }
  throw std::runtime_error("No target policy is found in JSON");
}

void JsonChanges::ExtractPresetMode() {
  if (p_jobj_ != nullptr && p_jobj_->contains("preset_mode") &&
      p_jobj_->at("preset_mode").is_string()) {
    preset_mode_ = p_jobj_->at("preset_mode").as_string().c_str();
    return;
  }
  throw std::runtime_error("No preset mode is found in JSON");
}

std::string JsonChanges::Process(bool apply) {
  if (preset_mode_ == "manual_mode") {
    ProcessManualMode();
  }
  if (preset_mode_ == "put_connected_to_white_list" ||
      preset_mode_ == "put_connected_to_white_list_plus_HID") {
    DeleteAllOldRules();
    ProcessAllowConnected();
    target_policy_ = Target::block;
  }
  if (preset_mode_ == "put_connected_to_white_list_plus_HID")
    AddAllowHid();
  if (preset_mode_ == "put_disks_and_mtp_to_black") {
    DeleteAllOldRules();
    AddBlockUsbStorages();
    target_policy_ = Target::allow;
  }
  if (preset_mode_ == "block_and_reject_android") {
    DeleteAllOldRules();
    AddBlockAndroid();
    target_policy_ = Target::allow;
  }

  obj_result_["rules_DELETED"] = json::array();
  for (const auto &deleted_rule_id : rules_deleted_) {
    obj_result_.at("rules_DELETED").as_array().push_back(deleted_rule_id);
  }
  if (obj_result_.contains("rules_BAD") &&
      !obj_result_.at("rules_BAD").as_array().empty()) {
    obj_result_["STATUS"] = "BAD";
  } else {
    obj_result_["STATUS"] = "OK";
  }

  if (!obj_result_.contains("rules_BAD")) {
    obj_result_["rules_BAD"] = json::array();
  }
  if (!obj_result_.contains("rules_OK")) {
    obj_result_["rules_OK"] = json::array();
  }

  obj_result_["rules_PRESET"] = AddAppendByPresetToResponse();

  // Log::Debug() << obj_result_;
  //  if we dont need to apply the rules
  if (!apply) {
    obj_result_["ACTION"] = "validation";
    return json::serialize(obj_result_);
  }
  obj_result_["ACTION"] = "apply";
  config_.ChangeImplicitPolicy(target_policy_ == Target::block);
  // add rules_to_add to new rules vector
  for (auto &rule : rules_to_add_)
    new_rules_.push_back(std::move(rule));
  std::string str_new_rules;
  for (const auto &rule : new_rules_) {
    str_new_rules += rule.BuildString(true, true);
    str_new_rules += "\n";
  }
  // Log::Debug() << "new rules " << str_new_rules;
  // overwrite the rules and test launch if some rules were added or deleted
  if (rules_changed_by_policy_ || !rules_to_delete_.empty() ||
      !rules_to_add_.empty()) {
    if (!config_.OverwriteRulesFile(str_new_rules, daemon_activate_)) {
      Log::Error() << "Can't launch the daemon with new rules";
      throw std::runtime_error("Can't launch the daemon with new rules");
    }
  }
  if (!config_.ChangeDaemonStatus(daemon_activate_, daemon_activate_))
    throw std::runtime_error("Change the daemon status FAILED");
  return json::serialize(obj_result_);
}

void JsonChanges::AddBlockAndroid() {
  const std::string path_to_vidpid = "/etc/usbguard/android_vidpid.json";
  try {
    if (!std::filesystem::exists(path_to_vidpid)) {
      throw(std::runtime_error("android_vidpid.json file doesn't exist"));
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't find file " << path_to_vidpid << "\n" << ex.what();
    throw std::runtime_error("android_vidpid file is missing");
  }
  std::stringstream buf;
  std::ifstream file_vid_pid(path_to_vidpid);
  if (!file_vid_pid.is_open()) {
    Log::Error() << "Can't open file" << path_to_vidpid;
    throw std::runtime_error("android_vidpid file is missing");
  }
  buf << file_vid_pid.rdbuf();
  file_vid_pid.close();

  boost::json::value json;
  boost::json::array *ptr_arr;
  try {
    json = boost::json::parse(buf.str());
  } catch (const std::exception &ex) {
    Log::Error() << "Can't parse a JSON file";
    throw std::runtime_error("Can't parse the android_vidpid file");
  }
  ptr_arr = json.if_array();
  if (ptr_arr == nullptr || ptr_arr->empty()) {
    Log::Error() << "Empty json array";
    throw std::runtime_error("An empty android_vidpid file");
  }
  for (const auto &element : *ptr_arr) {
    if (element.is_object() && element.as_object().contains("vid") &&
        element.as_object().contains("pid")) {
      std::stringstream string_builder;
      string_builder << "block id "
                     << common_utils::UnQuote(
                            element.as_object().at("vid").as_string().c_str())
                     << ":"
                     << common_utils::UnQuote(
                            element.as_object().at("pid").as_string().c_str());
      try {
        rules_to_add_.emplace_back(string_builder.str());
        added_by_preset_.emplace_back(string_builder.str());
      } catch (const std::logic_error &ex) {
        Log::Warning() << "Error creating a rule from JSON";
        Log::Warning() << string_builder.str();
        throw std::logic_error("Error creating a rule from JSON");
      }
    }
  }
}

boost::json::object JsonChanges::AddAppendByPresetToResponse() const {
  json::object res;
  if (!added_by_preset_.empty()) {
    res["hash"] = json::array();
    res["interface"] = json::array();
    res["vidpid"] = json::array();
    res["raw"] = json::array();
    for (const auto &rule : added_by_preset_) {
      switch (rule.level()) {
      case StrictnessLevel::hash: {
        json::object tmp_hash;
        tmp_hash["hash"] = common_utils::UnQuote(rule.hash());
        tmp_hash["description"] = common_utils::UnQuote(rule.device_name());
        tmp_hash["target"] = static_cast<uint>(rule.target());
        res["hash"].as_array().emplace_back(std::move(tmp_hash));
      } break;
      case guard::StrictnessLevel::interface: {
        json::object tmp_interface;
        tmp_interface["interface"] = rule.InterfacesToString();
        tmp_interface["target"] = static_cast<uint>(rule.target());
        res["interface"].as_array().emplace_back(std::move(tmp_interface));
      } break;
      case guard::StrictnessLevel::vid_pid: {
        json::object tmp_vidpid;
        tmp_vidpid["vid"] = rule.vid().value_or("");
        tmp_vidpid["pid"] = rule.pid().value_or("");
        tmp_vidpid["target"] = static_cast<uint>(rule.target());
        res["vidpid"].as_array().emplace_back(std::move(tmp_vidpid));
      } break;
      case guard::StrictnessLevel::non_strict: {
        json::object tmp_raw;
        tmp_raw["raw"] = rule.BuildString();
        tmp_raw["target"] = static_cast<uint>(rule.target());
        res["raw"].as_array().emplace_back(std::move(tmp_raw));
      } break;
      }
    }
  }
  return res;
}

void JsonChanges::AddBlockUsbStorages() {
  try {
    rules_to_add_.emplace_back("block with-interface 08:*:*");
    added_by_preset_.emplace_back("block with-interface 08:*:*");
    rules_to_add_.emplace_back("block with-interface 06:*:*");
    added_by_preset_.emplace_back("block with-interface 06:*:*");
    rules_to_add_.emplace_back("block with-interface one-of { 06:*:* 08:*:* }");
    added_by_preset_.emplace_back(
        "block with-interface one-of { 06:*:* 08:*:* }");
  } catch (const std::logic_error &ex) {
    Log::Error() << "Can't add a rules for USB storages";
    Log::Error() << ex.what();
    throw;
  }
}

void JsonChanges::AddAllowHid() {
  try {
    rules_to_add_.emplace_back("allow with-interface 03:*:*");
    rules_to_add_.emplace_back("allow with-interface 09:*:*");
    added_by_preset_.emplace_back("allow with-interface 03:*:*");
    added_by_preset_.emplace_back("allow with-interface 09:*:*");
  } catch (const std::logic_error &ex) {
    Log::Error() << "Can't add a rules for HID devices";
    Log::Error() << ex.what();
    throw;
  }
};

void JsonChanges::ProcessAllowConnected() {
  if (!active_devices_.has_value()) {
    throw std::logic_error("No active devices list found");
  }
  for (const UsbDevice &dev : active_devices_.value()) {
    std::stringstream string_builder;
    string_builder << "allow name "
                   << common_utils::QuoteIfNotQuoted(dev.name()) << " hash "
                   << common_utils::QuoteIfNotQuoted(dev.hash());
    try {
      rules_to_add_.emplace_back(string_builder.str());
      added_by_preset_.emplace_back(string_builder.str());
    } catch (const std::logic_error &ex) {
      Log::Error() << "Can't create a rule for device"
                   << "allow name "
                   << common_utils::QuoteIfNotQuoted(dev.name()) << " hash "
                   << common_utils::QuoteIfNotQuoted(dev.hash());
      Log::Error() << ex.what();
      throw;
    }
  }
}

void JsonChanges::ProcessManualMode() {
  using common_utils::StrToUint;
  obj_result_["rules_OK"] = json::array();
  obj_result_["rules_BAD"] = json::array();
  if (p_jobj_ != nullptr && p_jobj_->contains("appended_rules"))
    ProcessJsonAppended();
  // if we need to remove some rules
  // put rules numbers to "rules_to_delete" vector
  if (p_jobj_ != nullptr && p_jobj_->contains("deleted_rules") &&
      p_jobj_->at("deleted_rules").is_array()) {
    for (const auto &element : p_jobj_->at("deleted_rules").as_array()) {
      if (!element.is_string())
        continue;
      auto id_rule = StrToUint(element.as_string().c_str());
      if (id_rule.has_value()) {
        rules_to_delete_.push_back(*id_rule);
      } else {
        throw std::runtime_error("Error parsing numeric rule number from json");
      }
    }
  }
  DeleteRules();
}

void JsonChanges::DeleteAllOldRules() {
  // make sure that old rules were successfully parsed
  std::pair<std::vector<guard::GuardRule>, uint> parsed_rules =
      config_.ParseGuardRulesFile();
  if (parsed_rules.first.size() != parsed_rules.second)
    throw std::logic_error(
        "The rules file is not completelly parsed, can't edit");
  for (const auto &rule : parsed_rules.first) {
    rules_deleted_.push_back(rule.number());
  }
}

void JsonChanges::DeleteRules() {
  // make sure that old rules were successfully parsed
  std::pair<std::vector<guard::GuardRule>, uint> parsed_rules =
      config_.ParseGuardRulesFile();
  if (parsed_rules.first.size() != parsed_rules.second)
    throw std::logic_error(
        "The rules file is not completelly parsed, can't edit");
  // copy old rules,except listed in rule_indexes
  std::set<uint> unique_indexes(rules_to_delete_.cbegin(),
                                rules_to_delete_.cend());
  for (const auto &rule : parsed_rules.first) {
    if (unique_indexes.count(rule.number()) == 0) {
      // skip rules with conflicting policy,place rest to new_rules
      // if implicit policy="allow" rules must be block or reject
      if ((target_policy_ == Target::allow &&
           (rule.target() == Target::block ||
            rule.target() == Target::reject)) ||
          (target_policy_ == Target::block && rule.target() == Target::allow)) {
        new_rules_.push_back(rule);
      } else {
        rules_changed_by_policy_ = true;
        rules_deleted_.push_back(rule.number());
      }
    } else {
      rules_deleted_.push_back(rule.number());
    }
  }
}

void JsonChanges::ProcessJsonAppended() {
  const json::array *ptr_json_array_rules =
      p_jobj_->at("appended_rules").if_array();
  if (ptr_json_array_rules == nullptr)
    throw std::logic_error("Invalid json appended rules array");
  if (ptr_json_array_rules->empty())
    return;
  boost::json::array json_arr_OK;
  boost::json::array json_arr_BAD;
  for (const auto &rule : *ptr_json_array_rules) {
    const boost::json::object *ptr_json_rule = rule.if_object();
    if (ptr_json_rule != nullptr && ptr_json_rule->contains("tr_id")) {
      const boost::json::string *tr_id = ptr_json_rule->at("tr_id").if_string();
      // try to build a rule
      try {
        guard::json::JsonRule json_rule(ptr_json_rule);
        GuardRule rule_new{json_rule.BuildString()};
        ;
        if (rule_new.target() == target_policy_) {
          throw std::logic_error("Redundant rule");
        }
        rules_to_add_.push_back(std::move(rule_new));
        if (tr_id != nullptr && !tr_id->empty()) {
          json_arr_OK.emplace_back(*tr_id);
        }
      }
      // if failed
      catch (const std::logic_error &ex) {
        Log::Error() << "Can't build the rule";
        Log::Error() << ex.what();
        if (tr_id != nullptr && !tr_id->empty()) {
          json_arr_BAD.emplace_back(*tr_id);
        }
      }
    }
  }
  obj_result_["rules_OK"] = std::move(json_arr_OK);
  obj_result_["rules_BAD"] = std::move(json_arr_BAD);
}

} // namespace guard::json