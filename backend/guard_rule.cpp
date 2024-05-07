#include "guard_rule.hpp"
#include "common_utils.hpp"
#include "guard_utils.hpp"
#include "log.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>

namespace guard {

using common_utils::EscapeQuotes;
using common_utils::Log;
using common_utils::QuoteIfNotQuoted;

// static
const std::map<Target, std::string> GuardRule::map_target{
    {Target::allow, "allow"},
    {Target::block, "block"},
    {Target::reject, "reject"}};
const std::map<RuleConditions, std::string> GuardRule::map_conditions{
    {RuleConditions::localtime, "localtime"},
    {RuleConditions::allowed_matches, "allowed-matches"},
    {RuleConditions::rule_applied, "rule-applied"},
    {RuleConditions::rule_applied_past, "rule-applied("},
    {RuleConditions::rule_evaluated, "rule-evaluated"},
    {RuleConditions::rule_evaluated_past, "rule-evaluated("},
    {RuleConditions::random, "random"},
    {RuleConditions::random_with_propability, "random("},
    {RuleConditions::always_true, "true"},
    {RuleConditions::always_false, "false"},
    {RuleConditions::no_condition, ""}};
const std::map<RuleOperator, std::string> GuardRule::map_operator{
    {RuleOperator::all_of, "all-of"},
    {RuleOperator::one_of, "one-of"},
    {RuleOperator::none_of, "none-of"},
    {RuleOperator::equals, "equals"},
    {RuleOperator::equals_ordered, "equals-ordered"},
    {RuleOperator::no_operator, ""}};

GuardRule::GuardRule(const std::string &raw_str) {
  // Log::Debug() << "parsing rule";
  // Log::Debug() << raw_str;
  std::logic_error ex_common("Cant parse rule string");
  // Split string to tokens.
  std::vector<std::string> tokens{utils::SplitRawRule(raw_str)};
  if (tokens.empty())
    throw ex_common;
  // Map strings to values/
  // The target is mandatory.
  auto it_target = std::find_if(
      map_target.cbegin(), map_target.cend(),
      [&tokens](const auto &element) { return element.second == tokens[0]; });
  if (it_target == map_target.cend()) {
    throw ex_common;
  }
  target_ = it_target->first;
  tokens.erase(tokens.begin());
  // Conditions may or may not exist in the string.
  // Parse conditions first beacuse the "allow-matched()" condition contains
  // nested rule
  cond_ = ParseConditions(tokens);
  // id (vid:pid)
  // token id MUST contain a ':' symbol
  std::optional<std::string> str_id =
      utils::ParseToken(tokens, "id", utils::VidPidValidator);
  if (str_id.has_value()) {
    size_t separator{0};
    if (str_id->find(':') != std::string::npos)
      separator = str_id->find(':');
    else
      throw std::logic_error("Bad vid_pid" + str_id.value_or(""));
    vid_ = str_id->substr(0, separator);
    pid_ = str_id->substr(separator + 1);
    if (vid_->empty() || pid_->empty())
      throw ex_common;
  }
  // The default predicat for validation
  std::function<bool(const std::string &)> default_predicat =
      [](const std::string &val) {
        return !IsReservedWord(val) && !boost::starts_with(val, "\\\"") &&
               !boost::ends_with(val, "\\\"");
      };
  // Hash, device_name, serial, port, and with_interface may or may not exist in
  // the string. default_predicat - Function is the default behavior of values
  // validating.
  // hash length MUST be > 7 symbols
  hash_ = utils::ParseToken(
      tokens, "hash", [](const std::string &val) { return val.size() > 7; });
  parent_hash_ =
      utils::ParseToken(tokens, "parent-hash",
                        [](const std::string &val) { return val.size() > 7; });
  device_name_ = utils::ParseToken(tokens, "name", default_predicat);
  serial_ = utils::ParseToken(tokens, "serial", default_predicat);
  port_ = ParseTokenWithOperator(tokens, "via-port", default_predicat);

  // if a rules contains  with-interface { i1, i2 } array - insert the "equals"
  // operator
  auto it_with_interface =
      std::find(tokens.begin(), tokens.end(), "with-interface");
  if (it_with_interface != tokens.end()) {
    ++it_with_interface;
    if (it_with_interface != tokens.end() && *it_with_interface == "{") {
      tokens.insert(it_with_interface, "equals");
    }
  }
  with_interface_ = ParseTokenWithOperator(tokens, "with-interface",
                                           utils::InterfaceValidator);
  conn_type_ = utils::ParseToken(tokens, "with-connect-type", default_predicat);

  DetermineStrictnessLevel();
  FinalValidator(tokens);
  // Log::Debug() << "RULE BUILT";
  // Log::Debug() << BuildString();
}

void GuardRule::FinalValidator(std::vector<std::string> &splitted) const {
  // check
  for (std::string &str : splitted)
    boost::trim(str);
  auto it_end =
      std::remove_if(splitted.begin(), splitted.end(),
                     [](const std::string &str) { return str.empty(); });
  splitted.erase(it_end, splitted.end());
  if (!splitted.empty()) {
    Log::Error() << "Not all token were parsed in the rule";
    Log::Error err;
    for (const auto &tok : splitted) {
      err << "token" << tok << " ";
    }
    throw std::logic_error("Not all tokens were parsed");
  }

  if (vid_.value_or("").empty() && (pid_.value_or("").empty()) &&
      (hash_.value_or("").empty()) && (parent_hash_.value_or("").empty()) &&
      (device_name_.value_or("").empty()) && (serial_.value_or("").empty()) &&
      !port_.has_value() && !with_interface_.has_value() &&
      !conn_type_.has_value() && !cond_.has_value()) {
    throw std::logic_error("Empty rule");
  }
}

void GuardRule::DetermineStrictnessLevel() noexcept {
  // Determine the stricness level
  // conditions,parent-hash and port are not used for hashing
  // if rule contains a port or a condition - this is a "raw" rule
  if (hash_ && !cond_ && !port_)
    level_ = StrictnessLevel::hash;
  else if (vid_ && pid_ && !cond_ && !port_)
    level_ = StrictnessLevel::vid_pid;
  else if (with_interface_ && !cond_ && !port_)
    level_ = StrictnessLevel::interface;
  else
    level_ = StrictnessLevel::non_strict;
}

std::optional<std::pair<RuleOperator, std::vector<std::string>>>
GuardRule::ParseTokenWithOperator(
    std::vector<std::string> &splitted, const std::string &name,
    const std::function<bool(const std::string &)> &predicat) {
  std::logic_error ex_common("Cant parse rule string");
  std::optional<std::pair<RuleOperator, std::vector<std::string>>> res;
  // find token
  auto it_name = std::find(splitted.cbegin(), splitted.cend(), name);
  if (it_name != splitted.cend()) {
    auto it_param = it_name;
    ++it_param;
    // if a value exists -> check may be it is an operator
    if (it_param == splitted.cend()) {
      Log::Error() << "Parsing error. No values for param " << name
                   << " found.";
      throw ex_common;
    }
    // check if first param is an operator
    auto it_operator = std::find_if(
        map_operator.cbegin(), map_operator.cend(),
        [&it_param](const std::pair<RuleOperator, std::string> &val) {
          return val.second == *it_param;
        });
    bool have_operator{false};
    have_operator = it_operator != map_operator.cend();
    //  If no operator is found, parse as usual param - value
    if (!have_operator) {
      std::optional<std::string> tmp_value =
          utils::ParseToken(splitted, name, predicat);
      if (tmp_value.has_value()) {
        res = {RuleOperator::no_operator, {}};
        res->second.push_back(*tmp_value);
      }
    }
    // If an operator is found,an array of values is expected.
    else {
      res = {it_operator->first, {}}; // Create a pair with an empty vector.
      auto it_range_end = utils::ParseCurlyBracesArray(
          it_param, splitted.cend(), predicat, res->second);
      splitted.erase(it_name, ++it_range_end);
    }
  }
  return res;
}

std::string GuardRule::ConditionsToString() const {
  std::string res;
  if (!cond_)
    return "";
  // check if any operator
  if (map_operator.count(cond_->first) != 0) {
    res += map_operator.at(cond_->first);
    res += " ";
    if (cond_->first != RuleOperator::no_operator)
      res += "{";
  }
  int counter = 0;
  for (const auto &rule_with_bool : cond_->second) {
    // look in conditions map
    if (map_conditions.count(rule_with_bool.second.first) == 0)
      continue;
    if (counter != 0)
      res += " ";
    // check for negotiation
    if (!rule_with_bool.first)
      res += '!';
    // write string representation of rule into res
    res += map_conditions.at(rule_with_bool.second.first);
    // check if any parameters exist
    if (rule_with_bool.second.second) {
      if (rule_with_bool.second.first == RuleConditions::localtime ||
          rule_with_bool.second.first == RuleConditions::allowed_matches)
        res += "(";
      res += *rule_with_bool.second.second;
      res += ")";
    }
    ++counter;
  }
  if (cond_->first != RuleOperator::no_operator)
    res += "}";
  return res;
}

std::string
GuardRule::BuildString(bool build_parent_hash,
                       bool with_interface_array_no_operator) const noexcept {
  std::ostringstream res;
  res << map_target.at(target_);
  if (vid_ && pid_)
    res << " id " << *vid_ << ":" << *pid_;
  if (serial_)
    res << " serial " << QuoteIfNotQuoted(*serial_);
  if (device_name_)
    res << " name " << QuoteIfNotQuoted(*device_name_);
  if (hash_)
    res << " hash " << QuoteIfNotQuoted(*hash_);
  if (build_parent_hash && parent_hash_)
    res << " parent-hash " << QuoteIfNotQuoted(*parent_hash_);
  if (port_) {
    res << " via-port ";
    res << PortsToString();
  }
  if (with_interface_) {
    res << " with-interface ";
    res << InterfacesToString(with_interface_array_no_operator);
  }
  if (conn_type_)
    res << " with-connect-type " << QuoteIfNotQuoted(*conn_type_);

  if (cond_) {
    res << " if " << ConditionsToString();
  }
  std::string result = res.str();
  boost::trim(result);
  // Log::Debug() << result;
  return result;
}

std::string GuardRule::PortsToString() const {
  std::stringstream string_builder;
  if (port_) {
    string_builder << map_operator.at(port_->first);
    if (port_->first != RuleOperator::no_operator) {
      string_builder << " { ";
      for (const auto &port_val : port_->second) {
        string_builder << QuoteIfNotQuoted(port_val) << " ";
      }
      string_builder << "} ";
    } else {
      string_builder << QuoteIfNotQuoted(port_->second[0]);
    }
  }
  return string_builder.str();
}

std::string
GuardRule::InterfacesToString(bool with_interface_array_no_operator) const {
  std::stringstream res;
  if (!with_interface_.has_value())
    return res.str();
  if (with_interface_->second.size() > 1) {
    if (!with_interface_array_no_operator ||
        with_interface_->first != RuleOperator::equals) {
      res << map_operator.at(with_interface_->first);
    }
    res << " {";
    for (const auto &interf : with_interface_->second) {
      res << " " << interf;
    }
    res << " }";
  } else if (with_interface_->second.size() == 1) {
    res << with_interface_->second[0];
  }
  return res.str();
}

vecPairs GuardRule::SerializeForLisp() const {
  vecPairs res;
  res.emplace_back("name", std::to_string(number_));
  // The most string rules
  if (level_ == StrictnessLevel::hash) {
    res.emplace_back("lbl_rule_hash",
                     hash_.has_value() ? EscapeQuotes(hash_.value()) : "");
    res.emplace_back("lbl_rule_desc", device_name_.has_value()
                                          ? EscapeQuotes(device_name_.value())
                                          : "");
  }
  // VID::PID
  if (level_ == StrictnessLevel::vid_pid) {
    res.emplace_back("lbl_rule_vid",
                     vid_.has_value() ? EscapeQuotes(*vid_) : "");
    res.emplace_back("lbl_rule_vendor", vendor_name_.has_value()
                                            ? EscapeQuotes(*vendor_name_)
                                            : "");
    res.emplace_back("lbl_rule_pid",
                     pid_.has_value() ? EscapeQuotes(*pid_) : "");
    res.emplace_back("lbl_rule_product", device_name_.has_value()
                                             ? EscapeQuotes(*device_name_)
                                             : "");
    res.emplace_back("lbl_rule_port",
                     port_.has_value() ? EscapeQuotes(PortsToString()) : "");
  }
  // interface
  if (level_ == StrictnessLevel::interface) {
    res.emplace_back(
        "lbl_rule_interface",
        with_interface_.has_value() ? EscapeQuotes(InterfacesToString()) : "");
    res.emplace_back("lbl_rule_desc", device_name_.has_value()
                                          ? EscapeQuotes(device_name_.value())
                                          : "");
    res.emplace_back("lbl_rule_port",
                     port_.has_value() ? EscapeQuotes(PortsToString()) : "");
  }
  // non-strict
  if (level_ == StrictnessLevel::non_strict) {
    res.emplace_back("lbl_rule_raw", EscapeQuotes(BuildString()));
  }
  res.emplace_back("lbl_rule_target", map_target.count(target_) != 0
                                          ? map_target.at(target_)
                                          : "");
  return res;
}

boost::json::object GuardRule::BuildJsonObject() const {
  namespace json = boost::json;
  json::object res;
  res["number"] = std::to_string(number_);
  res["vid"] = vid_.value_or("");
  res["pid"] = pid_.value_or("");
  res["hash"] = EscapeQuotes(hash_.value_or(""));
  res["parent_hash"] = parent_hash_.value_or("");
  res["name"] = device_name_.value_or("");
  res["serial"] = serial_.value_or("");
  res["port"] = EscapeQuotes(PortsToString());
  res["interface"] = InterfacesToString(true);
  res["connect"] = conn_type_.value_or("");
  res["cond"] = ConditionsToString();
  res["level"] = static_cast<int>(StrictnessLevel::hash);
  res["policy"] = static_cast<uint>(target_);
  res["vendor_name"] = vendor_name_.value_or("");
  res["raw"] = EscapeQuotes(BuildString(true, true));
  return res;
}

StrictnessLevel
GuardRule::StrToStrictnessLevel(const std::string &str) noexcept {
  if (str == "hash")
    return StrictnessLevel::hash;
  if (str == "vid_pid")
    return StrictnessLevel::vid_pid;
  if (str == "interface")
    return StrictnessLevel::interface;
  return StrictnessLevel::non_strict;
}

std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>>
GuardRule::ParseConditions(std::vector<std::string> &splitted) {
  std::logic_error ex_common("Cant parse conditions");
  std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>> res;
  // Check if there are any conditions - look for "if"
  auto it_if_operator = std::find(splitted.cbegin(), splitted.cend(), "if");
  if (it_if_operator == splitted.cend())
    return std::nullopt;
  // Check if there is any operator
  bool have_operator{false};
  auto it_param1 = it_if_operator;
  ++it_param1;
  if (it_param1 == splitted.cend()) {
    Log::Error() << "No value was found for condition";
    throw ex_common;
  }
  auto it_operator = std::find_if(
      map_operator.cbegin(), map_operator.cend(),
      [&it_param1](const std::pair<RuleOperator, std::string> &val) {
        return val.second == *it_param1;
      });
  have_operator = it_operator != map_operator.cend();
  // No operator was found
  if (!have_operator) {
    std::vector<RuleWithBool> tmp;
    tmp.push_back(ParseOneCondition(it_param1, splitted.cend()));
    res = {RuleOperator::no_operator, std::move(tmp)};
    ++it_param1;
    if (it_param1 != splitted.cend()) {
      throw std::logic_error("Some text was found after a condition");
    }
    splitted.erase(it_if_operator, it_param1);
  } else {
    RuleOperator rule_operator = it_operator->first; // goes to the result
    auto range_begin = it_param1;
    ++range_begin;
    if (range_begin == splitted.cend() || *range_begin != "{") {
      throw std::logic_error("{ expected");
    }
    ++range_begin;
    if (range_begin == splitted.cend()) {
      throw ex_common;
    }
    auto range_end = std::find(range_begin, splitted.cend(), "}");
    if (range_end == splitted.cend() || range_begin >= range_end) {
      throw std::logic_error("} expected");
    }
    if (std::distance(range_begin, range_end) == 1) {
      throw std::logic_error("Empty array {} is not supported");
    }
    std::vector<RuleWithBool> tmp;
    for (auto it = range_begin; it != range_end; ++it) {
      tmp.push_back(ParseOneCondition(it, range_end));
      // it iterator will be incremented by parsing function
      // check bounds after parsing
      if (it == range_end)
        break;
    }
    ++range_end;
    if (range_end != splitted.cend()) {
      throw std::logic_error("Some text was found after conditions array");
    }
    splitted.erase(it_if_operator, range_end);
    res = {rule_operator, std::move(tmp)};
  }
  return res;
}

RuleWithBool GuardRule::ParseOneCondition(
    std::vector<std::string>::const_iterator &it_range_beg,
    std::vector<std::string>::const_iterator it_range_end) {
  RuleWithBool res;
  // Check for exclamation point
  bool exclamation_point = *it_range_beg == "!"; // goes to result
  if (exclamation_point) {
    ++it_range_beg;
  }
  if (it_range_beg == it_range_end)
    throw std::logic_error("Cant parse conditions");
  auto it_condition =
      std::find_if(map_conditions.cbegin(), map_conditions.cend(),
                   [&it_range_beg](const auto &pair) {
                     if (it_range_beg->size() < 2) {
                       return false;
                     } // just in case a brace trapped
                     return boost::contains(pair.second, *it_range_beg);
                   });
  if (it_condition == map_conditions.cend())
    throw std::logic_error("Cant parse this condition - " + *it_range_beg);
  RuleConditions condition = it_condition->first; // goes to result
  // Check if condition may have parameters
  bool may_have_params = utils::CanConditionHaveParams(condition);
  bool must_have_params = utils::MustConditionHaveParams(condition);
  // The condition, exclamation and may_have parameters are known
  ++it_range_beg;
  RuleWithOptionalParam rule_with_param{condition, std::nullopt};
  // Check if there any parameters
  if (it_range_beg != it_range_end && may_have_params) {
    std::string par_value = utils::ParseConditionParameter(
        it_range_beg, it_range_end, must_have_params);
    // if a parameter found
    if (!par_value.empty()) {
      rule_with_param.first = utils::ConvertToConditionWithParam(condition);
      rule_with_param.second = par_value;
      // move iterator to the end of current token
      while (it_range_beg != it_range_end && *it_range_beg != ")") {
        ++it_range_beg;
      }
    }
  } else if (it_range_beg == it_range_end && must_have_params) {
    --it_range_beg;
    throw std::logic_error("No parameters found for condition " +
                           *it_range_beg);
  }
  // move iterator back if no params were parsed
  if (!rule_with_param.second.has_value())
    --it_range_beg;
  res = {!exclamation_point, std::move(rule_with_param)};
  return res;
}

bool GuardRule::IsReservedWord(const std::string &str) noexcept {
  if (str.empty())
    return false;
  if (*str.cbegin() == '\"')
    return false;
  if (std::find_if(map_target.cbegin(), map_target.cend(),
                   [&str](const auto &pair) { return pair.second == str; }) !=
      map_target.cend())
    return true;
  if (std::find_if(map_conditions.cbegin(), map_conditions.cend(),
                   [&str](const auto &pair) { return pair.second == str; }) !=
      map_conditions.cend())
    return true;
  if (std::find_if(map_operator.cbegin(), map_operator.cend(),
                   [&str](const auto &pair) { return pair.second == str; }) !=
      map_operator.cend())
    return true;
  return false;
}

} // namespace guard