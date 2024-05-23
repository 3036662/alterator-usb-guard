#pragma once
#include "serializable_for_lisp.hpp"
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#ifdef UNIT_TEST
#include "test.hpp"
#endif

namespace guard {

// clang-format off

/// @brief Target ::= allow | block | reject
enum class Target { allow, block, reject };


/// @brief Operators used in usbguard rules
enum class RuleOperator {
  all_of,  // Evaluate to true if all of the specified conditions evaluated to true.
  one_of,  // Evaluate to true if one of the specified conditions evaluated to true.
  none_of, // Evaluate to true if none of the specified conditions evaluated to true.
  equals,  //  Same as all-of.
  equals_ordered, //  Same as all-of.
  no_operator     // if no operators are used
};


/// @brief Conditions used in usbguard rules
enum class RuleConditions {
  localtime,    //  Evaluates to true if the local time is in the specified time range.
  allowed_matches, // Evaluates to true if an allowed device matches the specified query.
  rule_applied, // Evaluates to true if the rule currently being evaluated ever matched a device.
  rule_applied_past, // Evaluates to true if the rule currently being evaluatedmatched a device in the past duration of time specified by the parameter.
  rule_evaluated, // Evaluates to true if the rule currently being evaluated was ever evaluated before.
  rule_evaluated_past, // Evaluates to true if the rule currently being evaluated was evaluated in the pas duration of timespecified by the parameter.
  random,       // Evaluates to true/false with a probability of p=0.5
  random_with_propability, // Evaluates to true with the specified probability p_true.
  always_true,  //  Evaluates always to true.
  always_false, // Evaluates always to false
  no_condition
};


/// @brief Strictness levels 
enum class StrictnessLevel{
  hash, // the most strict
  vid_pid, 
  interface,
  non_strict
};

// clang-format on

using RuleWithOptionalParam =
    std::pair<RuleConditions, std::optional<std::string>>;
using RuleWithBool = std::pair<bool, RuleWithOptionalParam>;

/**
 * @brief Parse and store rules from usbguard rules.conf file
 * @class GuardRule
 * @throws Constructor - std::logical_error
 */
class GuardRule : public SerializableForLisp<GuardRule> {
public:
  /**
   * @brief Construct a new Guard Rule object
   * @param str String rule from usbguard rules file
   * @throws std::logical_error
   */
  explicit GuardRule(const std::string &raw_str);

  // GuardRule &operator=(const GuardRule &) noexcept = delete;
  // GuardRule &operator=(GuardRule &&) noexcept = delete;
  // GuardRule(GuardRule &&)  = default;
  // GuardRule(const GuardRule &)  = default;

  /**
   * @brief Build rule string for usbguard
   *
   * @param build_parent_hash include parent hash to rule string
   * @param false skip operator "equals" for an array of interface
   * @return std::string
   * @details The parameters are used to build the same rule as usbguard cli
   * builds
   */
  std::string
  BuildString(bool build_parent_hash = true,
              bool with_interface_array_no_operator = true) const noexcept;

  /**
   * @brief Builds a vector of string pairs, suitable for
   * sending to lisp
   * @return std::vector<std::string,std::string>
   */
  vecPairs SerializeForLisp() const;

  boost::json::object BuildJsonObject() const;

  /// @brief Converts a string representation of Rule StricnessLevel to a
  /// StrictnessLevel
  /// @param str Stricness level ("hash","vid_pid","interface");
  /// @return StrictnessLevel - non_strict if no corresondent level is found.
  static StrictnessLevel StrToStrictnessLevel(const std::string &str) noexcept;

  /// @brief Builds a string from interfaces
  std::string
  InterfacesToString(bool with_interface_array_no_operator = true) const;

  inline const std::optional<std::string> &vid() const noexcept {
    return vid_;
  };
  inline const std::optional<std::string> &pid() const noexcept {
    return pid_;
  };
  inline const std::optional<std::string> &vendor_name() const noexcept {
    return vendor_name_;
  };
  inline void vendor_name(const std::string &v_name) noexcept {
    vendor_name_ = v_name;
  };
  inline StrictnessLevel level() const noexcept { return level_; };
  inline void level(StrictnessLevel new_level) noexcept { level_ = new_level; };
  inline uint number() const noexcept { return number_; };
  inline void number(uint numb) noexcept { number_ = numb; };
  inline Target target() const noexcept { return target_; };
  inline std::string hash() const noexcept { return hash_.value_or(""); }
  inline std::string device_name() const noexcept {
    return device_name_.value_or("");
  }

private:
  ///  @brief Build a condition string from this object.
  std::string ConditionsToString() const;
  /// @brief Determine a strictness level, wtite to level_
  void DetermineStrictnessLevel() noexcept;
  /**
   * @brief Finial validation of builded rule
   * @throw std::logic_error
   */
  void FinalValidator(std::vector<std::string> &) const;
  /// @brief Builds a string from ports
  std::string PortsToString() const;

  /**
   * @brief Parse a token when operators are possible for token
   *
   * @param splitted Vector - a splitted rule string.
   * @param name String name of a parameter to look for.
   * @param predicat bool(sting&) function returning true, if string is valid
   * value
   * @return std::optional<std::pair<RuleOperator, std::vector<std::string>>>
   * value for parameter
   */
  static std::optional<std::pair<RuleOperator, std::vector<std::string>>>
  ParseTokenWithOperator(
      std::vector<std::string> &splitted, const std::string &name,
      const std::function<bool(const std::string &)> &predicat);

  /**
   * @brief Parse conditions
   *
   * @param splitted Vector - a splitted rule string.
   * @return std::optional<
   * std::pair<RuleOperator, std::vector<std::pair<RuleConditions, bool>>>>
   */
  static std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>>
  ParseConditions(std::vector<std::string> &splitted);

  /**
   * @brief Parses one condition with or without params
   *
   * @param it_range_beg An iterator, pointing to a token where to start.
   * @param it_range_end An iterator, pointing to the end of token sequence.
   * @return !Rule -> <false,Rule> | Rule -> <true,Rule>
   * @warning Changes a velue of begin iterator.
   * A way to inform the calling function how many tokens were read during
   * a parse process.
   */
  static RuleWithBool
  ParseOneCondition(std::vector<std::string>::const_iterator &it_range_beg,
                    std::vector<std::string>::const_iterator it_range_end);

  /**
   * @brief Checks if sring looks like one of parameter reserve words for rule
   *
   * @param str parameter
   * @return true if string is normal value
   * @return false if string looks like reserved word
   */
  static bool IsReservedWord(const std::string &str) noexcept;

  /* TODO This varibles should be refactored to consexpr if start using
   * multithreading*/
  /// @brief Maps Target enum to a string.
  static const std::map<Target, std::string> map_target;
  /// @brief Maps RuleConditions enum to a string.
  static const std::map<RuleConditions, std::string> map_conditions;
  /// @brief Maps RuleOperator enum to a string.
  static const std::map<RuleOperator, std::string> map_operator;

  uint number_ = 0; ///@brief number of line (from the beginnig of file)
  Target target_;
  std::optional<std::string> vid_;
  std::optional<std::string> pid_;
  std::optional<std::string> hash_;
  std::optional<std::string> parent_hash_;
  std::optional<std::string> device_name_;
  std::optional<std::string> serial_;
  std::optional<std::pair<RuleOperator, std::vector<std::string>>> port_;
  std::optional<std::pair<RuleOperator, std::vector<std::string>>>
      with_interface_;
  std::optional<std::string> conn_type_;
  std::optional<std::pair<RuleOperator, std::vector<RuleWithBool>>> cond_;
  StrictnessLevel level_ = StrictnessLevel::hash;
  std::optional<std::string> vendor_name_;

#ifdef UNIT_TEST
  friend class ::Test;
#endif
};

} // namespace guard