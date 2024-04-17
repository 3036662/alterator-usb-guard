#include "guard_utils.hpp"
#include "base64_rfc4648.hpp"
#include "csv_rule.hpp"
#include "guard_rule.hpp"
#include "json_rule.hpp"
#include "log.hpp"
#include "rapidcsv.h"
#include "usb_device.hpp"
#include "utils.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <stdexcept>
#include <sys/types.h>
#include <utility>

namespace guard::utils {

std::optional<std::vector<GuardRule>>
UploadRulesCsv(const std::string &file) noexcept {
  const size_t kColumnsRequired = 2;
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

std::optional<std::string>
BuildJsonArrayOfUpploaded(const std::vector<GuardRule> &vec_rules) noexcept {
  namespace json = boost::json;
  try {
    json::object res_obj;
    res_obj["hash_rules"] = json::array();
    res_obj["vidpid_rules"] = json::array();
    res_obj["interf_rules"] = json::array();
    res_obj["raw_rules"] = json::array();
    bool found_conflicting = false;
    Target common_target_policy = Target::allow;
    if (!vec_rules.empty())
      common_target_policy = vec_rules[0].target();
    res_obj["policy"] = static_cast<uint>(common_target_policy);
    for (const auto &rule : vec_rules) {
      if (rule.target() != common_target_policy)
        found_conflicting = true;
      // Log::Debug() << rule.BuildJsonObject();
      switch (rule.level()) {
      case StrictnessLevel::hash:
        res_obj.at("hash_rules").as_array().push_back(rule.BuildJsonObject());
        break;
      case StrictnessLevel::interface:
        res_obj.at("interf_rules").as_array().push_back(rule.BuildJsonObject());
        break;
      case StrictnessLevel::vid_pid:
        res_obj.at("vidpid_rules").as_array().push_back(rule.BuildJsonObject());
        break;
      case StrictnessLevel::non_strict:
        // must be empty for now - because loading of raw rules from csv is
        // unsupported
        // res_obj.at("raw_rules").as_array().push_back(rule.BuildJsonObject());
        break;
      }
    }
    if (found_conflicting) {
      res_obj["STATUS"] = "error";
      res_obj["ERR_MSG"] = "Conflicting rule targets";
      return json::serialize(res_obj);
    }
    res_obj["STATUS"] = "OK";
    return json::serialize(res_obj);
  } catch (const std::exception &ex) {
    Log::Error() << "Error building json list of rules";
    return std::nullopt;
  }
  Log::Debug() << "No exceptions but no JSON was build ";
  return std::nullopt;
}

std::vector<std::string> FoldUsbInterfacesList(std::string i_type) {
  boost::erase_first(i_type, "with-interface");
  boost::trim(i_type);
  std::vector<std::string> vec_i_types;
  // if a multiple types in one string
  if (i_type.find('{') != std::string::npos &&
      i_type.find('}') != std::string::npos) {
    std::vector<std::string> splitted;
    boost::erase_all(i_type, "{");
    boost::erase_all(i_type, "}");
    boost::trim(i_type);
    boost::split(splitted, i_type,
                 [](const char symbol) { return symbol == ' '; });
    // create sequence of usb types
    std::vector<UsbType> vec_usb_types;
    for (const std::string &usb_type : splitted) {
      try {
        vec_usb_types.emplace_back(usb_type);
      } catch (const std::exception &e) {
        Log::Error() << "Can't parse a usb type" << usb_type;
        Log::Error() << e.what();
      }
    }
    // fold if possible
    // put to multiset bases
    std::multiset<unsigned char> set;
    for (const UsbType &usb_type : vec_usb_types) {
      set.emplace(usb_type.base());
    }
    // if a key is not unique, create a mask.
    auto it_unique_end =
        std::unique(vec_usb_types.begin(), vec_usb_types.end(),
                    [](const UsbType &first, const UsbType &second) {
                      return first.base() == second.base();
                    });
    for (auto it = vec_usb_types.begin(); it != it_unique_end; ++it) {
      size_t numb = set.count(it->base());
      std::string tmp = it->base_str();
      if (numb == 1) {
        tmp += ':';
        tmp += it->sub_str();
        tmp += ':';
        tmp += it->protocol_str();
      } else {
        tmp += ":*:*";
      }
      vec_i_types.emplace_back(std::move(tmp));
    }
  } else {
    try {
      UsbType tmp(i_type);
    } catch (const std::exception &ex) {
      Log::Error() << "Can't parse a usb type " << i_type;
      Log::Error() << ex.what();
    }
    // push even if parsing failed
    vec_i_types.push_back(std::move(i_type));
  }
  return vec_i_types;
}

std::unordered_map<std::string, std::string>
MapVendorCodesToNames(const std::unordered_set<std::string> &vendors) noexcept {
  using guard::utils::Log;
  std::unordered_map<std::string, std::string> res;
  const std::string path_to_usb_ids = "/usr/share/misc/usb.ids";
  try {
    if (std::filesystem::exists(path_to_usb_ids)) {
      std::ifstream filestream(path_to_usb_ids);
      if (filestream.is_open()) {
        std::string line;
        while (getline(filestream, line)) {
          // not interested in strings starting with tab
          if (!line.empty() && line[0] == '\t') {
            line.clear();
            continue;
          }
          auto range = boost::find_first(line, "  ");
          if (static_cast<bool>(range)) {
            std::string vendor_id(line.begin(), range.begin());
            if (vendors.count(vendor_id) != 0) {
              std::string vendor_name(range.end(), line.end());
              res.emplace(std::move(vendor_id), std::move(vendor_name));
            }
          }
          line.clear();
        }
        filestream.close();
      } else {
        Log::Warning() << "Can't open file " << path_to_usb_ids;
      }
    } else {
      Log::Error() << "The file " << path_to_usb_ids << "doesn't exist";
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't map vendor IDs to vendor names.";
    Log::Error() << ex.what();
  }
  return res;
}

/*----------------- GuardRule utility functions ----------------- */

bool VidPidValidator(const std::string &val) noexcept {
  if (val.find(':') == std::string::npos)
    return false;
  std::vector<std::string> spl;
  try {
    boost::split(spl, val, [](const char symbol) { return symbol == ':'; });
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
    return false;
  }
  if (spl.size() != 2)
    return false;
  if (spl[0] == "*" && spl[1] != "*")
    return false;
  for (const auto &element : spl) {
    if (element.size() > 4)
      return false;
    if (element.size() < 4) {
      return element == "*";
    }
    try {
      size_t pos = 0;
      std::stoi(element, &pos, 16);
      if (pos != element.size())
        throw std::logic_error("Not HEX number");
    } catch (const std::exception &ex) {
      Log::Error() << "Can't parse id " << element;
      return false;
    }
  }
  return true;
}

bool InterfaceValidator(const std::string &val) noexcept {
  std::vector<std::string> spl;
  try {
    boost::split(spl, val, [](const char symbol) { return symbol == ':'; });
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
    return false;
  }
  if (spl.size() != 3)
    return false;
  if (spl[0] == "*" && (spl[1] != "*" || spl[2] != "*"))
    return false;
  if (spl[1] == "*" && spl[2] != "*")
    return false;
  for (const auto &element : spl) {
    if (element.size() != 2 && element == "*")
      return true;
    try {
      size_t pos = 0;
      std::stoi(element, &pos, 16);
      if (pos != element.size())
        throw std::logic_error("Not HEX number");
    } catch (const std::exception &ex) {
      Log::Error() << "Can't parse interface " << val;
      return false;
    }
  }
  return true;
}

std::vector<std::string> SplitRawRule(std::string raw_str) noexcept {
  boost::trim(raw_str);
  std::vector<std::string> res;
  if (raw_str.empty())
    return res;
  utils::WrapBracesWithSpaces(raw_str);
  // Split string with spaces.
  // I current space is a part of a quoted string - skip it.
  auto it_slow = raw_str.begin();
  auto it_fast = it_slow;
  ++it_fast;
  bool dont_split{raw_str[0] == '\"'};
  while (it_fast != raw_str.end()) {
    if (*it_fast == ' ' && !dont_split) {
      res.emplace_back(it_slow, it_fast);
      while (it_fast != raw_str.end() && *it_fast == ' ')
        ++it_fast;
      it_slow = it_fast;
    }
    if (it_fast != raw_str.end() && *it_fast == '\"') {
      dont_split = !dont_split;
    }
    if (it_fast != raw_str.end())
      ++it_fast;
  }
  if (it_fast == raw_str.end() && !dont_split && it_slow < it_fast)
    res.emplace_back(it_slow, it_fast);
  for (std::string &str : res)
    boost::trim(str);
  std::vector<std::string> tmp;
  for (auto &str : res) {
    if (!res.empty())
      tmp.emplace_back(std::move(str));
  }
  std::swap(res, tmp);
  return res;
}

void WrapBracesWithSpaces(std::string &raw_str) noexcept {
  std::string tmp;
  // if symbol is a part of a quouted string - dont wrap
  bool dont_wrap{raw_str[0] == '\"'};
  for (auto it = raw_str.begin(); it != raw_str.end(); ++it) {
    bool wrap =
        *it == '{' || *it == '}' || *it == '!' || *it == '(' || *it == ')';
    if (!dont_wrap && wrap) {
      tmp.push_back(' ');
    }
    tmp.push_back(*it);
    if (!dont_wrap && wrap) {
      tmp.push_back(' ');
    }
    if (*it == '\"') {
      dont_wrap = !dont_wrap;
    }
  }
  std::swap(raw_str, tmp);
}

std::optional<std::string>
ParseToken(std::vector<std::string> &splitted, const std::string &name,
           const std::function<bool(const std::string &)> &predicat) {
  std::optional<std::string> res;
  auto it_name = std::find(splitted.cbegin(), splitted.cend(), name);
  if (it_name != splitted.cend()) {
    auto it_name_param = it_name;
    ++it_name_param;
    if (it_name_param != splitted.cend() && predicat(*it_name_param)) {
      res = *it_name_param;
    } else {
      Log::Error() << "Parsing error, token " << *it_name_param;
      throw std::logic_error("Cant parse rule string");
    }
    splitted.erase(it_name, ++it_name_param);
  }
  return res;
}

std::string
ParseConditionParameter(std::vector<std::string>::const_iterator it_start,
                        std::vector<std::string>::const_iterator it_end,
                        bool must_have_params) {
  std::logic_error ex_common("Can't parse parameters for condition");
  // Parse parameters.
  auto it_open_round_brace = it_start;
  if (*it_open_round_brace != "(") {
    if (must_have_params) {
      throw std::logic_error("( expected");
    }
    return "";
  }
  ++it_start;

  std::string res;
  while (it_start != it_end && *it_start != ")") {
    res += *it_start;
    res += " ";
    ++it_start;
  }
  // if a closing brace was not found
  if (it_start == it_end) {
    throw ex_common;
  }
  auto it_close_round_brace = it_start;
  if (*it_close_round_brace != ")") {
    throw ex_common;
  }
  boost::trim(res);
  return res;
}

std::vector<std::string>::const_iterator
ParseCurlyBracesArray(std::vector<std::string>::const_iterator it_range_begin,
                      std::vector<std::string>::const_iterator it_end,
                      const std::function<bool(const std::string &)> &predicat,
                      std::vector<std::string> &res_array) {
  std::logic_error ex_common("Cant parse rule string");
  ++it_range_begin;
  // if no array found -> throw exception
  if (it_range_begin == it_end || *it_range_begin != "{") {
    // Log::Error() << "Error parsing values for " << name << " param.";
    throw ex_common;
  }
  auto it_range_end = std::find(it_range_begin, it_end, "}");
  if (it_range_end == it_end) {
    Log::Error() << "A closing \"}\" expectend for sequence.";
    throw ex_common;
  }
  if (std::distance(it_range_begin, it_range_end) == 1) {
    throw std::logic_error("Empty array {} is not supported");
  }
  // fill result with values
  auto it_val = it_range_begin;
  ++it_val;
  while (it_val != it_range_end) {
    if (predicat(*it_val)) {
      res_array.emplace_back(*it_val);
    } else {
      Log::Error() << "Parsing error, token " << *it_val;
      throw ex_common;
    }
    ++it_val;
  }
  return it_range_end;
}

bool CanConditionHaveParams(RuleConditions cond) noexcept {
  return cond == RuleConditions::localtime ||
         cond == RuleConditions::allowed_matches ||
         cond == RuleConditions::rule_applied ||
         cond == RuleConditions::rule_applied_past ||
         cond == RuleConditions::rule_evaluated ||
         cond == RuleConditions::rule_evaluated_past ||
         cond == RuleConditions::random;
}

bool MustConditionHaveParams(RuleConditions cond) noexcept {
  return cond == RuleConditions::localtime ||
         cond == RuleConditions::allowed_matches ||
         cond == RuleConditions::rule_applied_past ||
         cond == RuleConditions::rule_evaluated_past;
}

RuleConditions ConvertToConditionWithParam(RuleConditions cond) noexcept {
  if (cond == RuleConditions::rule_applied)
    return RuleConditions::rule_applied_past;
  if (cond == RuleConditions::rule_evaluated)
    return RuleConditions::rule_evaluated_past;
  if (cond == RuleConditions::random)
    return RuleConditions::random_with_propability;
  return cond;
}

/*------------------ ConfigStatus free-standing util functions ------------*/

bool IsSuspiciousUdevFile(const std::string &str_path) {
  std::ifstream file_udev_rule(str_path);
  if (file_udev_rule.is_open()) {
    std::string tmp_str;
    // bool found_usb{false};
    bool found_authorize{false};
    // for each string
    while (getline(file_udev_rule, tmp_str)) {
      // case insentitive search
      std::transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(),
                     [](unsigned char symbol) {
                       return std::isalnum(symbol) != 0 ? std::tolower(symbol)
                                                        : symbol;
                     });
      // if (tmp_str.find("usb") != std::string::npos) {
      //   found_usb = true;
      // }
      if (tmp_str.find("authorize") != std::string::npos) {
        found_authorize = true;
      }
    }
    tmp_str.clear();
    file_udev_rule.close();
    // if (found_usb && found_authorize) {
    // a rule can ruin program behavior even if only authorized and no usb
    if (found_authorize) {
      Log::Info() << "Found file " << str_path;
      return true;
    }
  } else {
    throw std::runtime_error("Can't inspect file " + str_path);
  }
  return false;
}

std::unordered_map<std::string, std::string> InspectUdevRules(
#ifdef UNIT_TEST
    const std::vector<std::string> *vec
#endif
    ) noexcept {
  std::unordered_map<std::string, std::string> res;
  std::vector<std::string> udev_paths{"/usr/lib/udev/rules.d",
                                      "/usr/local/lib/udev/rules.d",
                                      "/run/udev/rules.d", "/etc/udev/rules.d"};
#ifdef UNIT_TEST
  if (vec != nullptr)
    udev_paths = *vec;
#endif
  // Log::Info() << "Inspecting udev folders";
  for (const std::string &path : udev_paths) {
    // Log::Info() << "Inspecting udev folder " << path;
    // find all files in folder
    std::vector<std::string> files =
        ::utils::FindAllFilesInDirRecursive({path, ".rules"});
    // for each file - check if it contains suspicious strings
    for (const std::string &str_path : files) {
      try {
        if (utils::IsSuspiciousUdevFile(str_path)) {
          Log::Info() << "Found file " << str_path;
          res.emplace(str_path, "usb_rule");
        }
      } catch (const std::exception &ex) {
        Log::Error() << ex.what();
      }
    }
  }
  return res;
}

} // namespace guard::utils