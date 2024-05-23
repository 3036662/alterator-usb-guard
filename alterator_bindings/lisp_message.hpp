#ifndef LISP_MESSAGE_HPP
#define LISP_MESSAGE_HPP

#include <iostream>
#include <string>
#include <unordered_map>

/**
 * @class LispMessage
 * @brief Represents a message from alterator frontend
 */
struct LispMessage {
  struct MsgAction {
    std::string val;
  };
  struct MsgObject {
    std::string val;
  };
  std::string action;
  std::string objects;
  std::unordered_map<std::string, std::string> params;
  /// @brief Constructor for an empty message
  LispMessage();
  /// @brief Constructor for full-fledged message
  /// @param act String action (read,list,etc.)
  /// @param obj Alterator path to backend
  /// @param prms A map of string pairs repres. parameters
  LispMessage(
      const MsgAction &act, const MsgObject &obj,
      const std::unordered_map<std::string, std::string> &prms) noexcept;
};

/// @brief  The output operator for the stream
/// @param os Stream
/// @param mes LispMessage object
/// @return Stream
std::ostream &operator<<(std::ostream &ostream, const LispMessage &mes);

#endif // LISP_MESSAGE_HPP
