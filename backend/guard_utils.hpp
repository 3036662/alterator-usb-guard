#pragma once

/**
 * @brief Usb-related free-standing functions
 */

#include "guard_rule.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace guard::utils {

/**
 * @brief Upload CSV rules from file, uploaded by user
 * @param file  file content
 */
std::optional<std::vector<GuardRule>>
UploadRulesCsv(const std::string &file) noexcept;

/**
 * @brief Build a json response for rules uploaded from csv
 */
std::optional<std::string>
BuildJsonArrayOfUpploaded(const std::vector<GuardRule> &vec_rules) noexcept;

///@brief fold list of interface { 03:01:02 03:01:01 } to [03:*:*]
///@param i_type string with list of interfaces from usbguard
std::vector<std::string> FoldUsbInterfacesList(std::string i_type);

/**
 * @brief Creates map vendor ID : vendor Name with in one pass to fil
 * @param vendors set of vendor IDs
 * @return std::map<std::string,std::string> Vendor ID : Vendor Name
 */
std::unordered_map<std::string, std::string>
MapVendorCodesToNames(const std::unordered_set<std::string> &vendors) noexcept;

/// @brief extract daemon target state from json (run || disabled)
std::optional<bool>
ExtractDaemonTargetState(boost::json::object *p_obj) noexcept;

/// @brief extract daemon target implicit polict from json (block || allow)
std::optional<Target> ExtractTargetPolicy(boost::json::object *p_obj) noexcept;

/// @brief extract preset type from json
std::optional<std::string>
ExtractPresetMode(boost::json::object *p_obj) noexcept;

/**
 * @brief Process rules for "manual" mode
 * @param ptr_jobj Json object, containig "appended_rules" and "deleted_rules"
 * arrays
 * @param[out] rules_to_delete array,where to put order numbers of rules to
 * delete
 * @param[out] rules_to_add array, where to put rules to append
 * @return boost::json::object, containig "rules_OK" and "rules_BAD" arrays
 * @details rules_OK and rules_BAD contains html <tr> ids for validation
 */
boost::json::object
ProcessJsonManualMode(const boost::json::object *ptr_jobj,
                      std::vector<uint> &rules_to_delete,
                      std::vector<GuardRule> &rules_to_add) noexcept;

/**
 * @brief Parses "appended" rules from json
 * @param[in] ptr_json_array_rules A pointer to json array with rules
 * @param rules_to_add Vector, where new rulles will be appended
 * @return JSON object, containig arrays of html ids - "rules_OK" and
 * "rules_BAD"
 * @details This function is called from ProcessJsonManualMode
 */
boost::json::object
ProcessJsonAppended(const boost::json::array *ptr_json_array_rules,
                    std::vector<GuardRule> &rules_to_add) noexcept;

/// @brief Add a rule to allow all HID devices.
/// @param rules_to_add Vector,where a new rule will be appended.
void AddAllowHid(std::vector<GuardRule> &rules_to_add) noexcept;

///@brief Add a rule to block 08 and 06 - usb and mtp.
/// @param rules_to_add Vector,where a new rule will be appended.
void AddBlockUsbStorages(std::vector<GuardRule> &rules_to_add) noexcept;

/// @brief Add rules to reject known android devices
/// @param rules_to_add Vector,where a new rule will be appended.
bool AddRejectAndroid(std::vector<GuardRule> &rules_to_add) noexcept;

/*----------------- GuardRule utility functions ----------------- */

/// @brief Validation predicate for a VID/PID value
bool VidPidValidator(const std::string &val) noexcept;
/// @brief Validation predicate for an interface value
bool InterfaceValidator(const std::string &val) noexcept;

/**
 * @brief Split string by space, dont split double-quouted substrings
 *
 * @param raw_str String to split
 * @return std::vector<std::string>
 * @details "id    1    name   "Long Name"" -> { "id","1",""Long Name""}
 */
std::vector<std::string> SplitRawRule(std::string raw_str) noexcept;

/// @brief  // Inline wrap all curly and round braces and exclamation points
/// with spaces
void WrapBracesWithSpaces(std::string &raw_str) noexcept;

/**
 * @brief Parse token
 *
 * @param splitted Vector - a splitted rule string.
 * @param name String name of a parameter to look for.
 * @param predicat bool(sting&) function returning true, if string is valid
 * value
 * @return std::optional<std::string> value for parameter
 */
std::optional<std::string>
ParseToken(std::vector<std::string> &splitted, const std::string &name,
           const std::function<bool(const std::string &)> &predicat);

/**
 * @brief Parses a condition parameter
 * A parameter is expected to be enclosed in round braces.
 *
 * @param it_start An iterator, pointing to a token where to start. "("
 * @param it_end  An iterator, pointing to the end of token sequence.
 * @return A string value of parameter.
 */
std::string
ParseConditionParameter(std::vector<std::string>::const_iterator it_start,
                        std::vector<std::string>::const_iterator it_end,
                        bool must_have_params = false);
/**
 * @brief Parses array {val1 val2 ...}
 *
 * @param it_range_begin Iterator to last token befor array
 * @param it_end Iterator to end bound of sequence
 * @param predicat Validator for a value
 * @param res_array Array where values must be appended
 * @return An iterator to the end of an array aka "}" token
 */
std::vector<std::string>::const_iterator
ParseCurlyBracesArray(std::vector<std::string>::const_iterator it_range_begin,
                      std::vector<std::string>::const_iterator it_end,
                      const std::function<bool(const std::string &)> &predicat,
                      std::vector<std::string> &res_array);

/**
 * @brief Checks if a condition is allowed to have parameters.
 *
 * @param cond RuleConditions
 * @return true
 * @return false
 */
bool CanConditionHaveParams(RuleConditions cond) noexcept;

/// @brief returns true if a condition must have parameters.
bool MustConditionHaveParams(RuleConditions cond) noexcept;

/**
 * @brief Choose an appropriate emun for RuleConditions
 * in case it has parameters.
 * @param cond
 * @return RuleConditions
 */
RuleConditions ConvertToConditionWithParam(RuleConditions cond) noexcept;

/*------------------ ConfigStatus free-standing util functions ------------*/

/**
 * @brief Incpect a udev rule file.
 * @throws std::runtime_error if can't open the file
 */
bool IsSuspiciousUdevFile(const std::string &str_path);

/// @brief  inspect udev rules for suspicious files
/// @param vec just for testing purposes
/// @return map of string file:warning
std::unordered_map<std::string, std::string> InspectUdevRules(
#ifdef UNIT_TEST
    const std::vector<std::string> *vec = nullptr
#endif
    ) noexcept;

} // namespace guard::utils