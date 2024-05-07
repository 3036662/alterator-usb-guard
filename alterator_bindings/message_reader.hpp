#ifndef MESSAGE_READER_HPP
#define MESSAGE_READER_HPP

#include "lisp_message.hpp"
#include "message_dispatcher.hpp"
#include <string>

/**
 * @class MessageReader
 * @brief Reads message from alterator frontend
 *
 * Creates LispMessage objects ans sends them to MessageDispatcher
 */
class MessageReader {
public:

  /**
   * @brief Construct a new Message Reader object
   * @param Function bool(*)(const LispMessage&) as dispatcher implementation
   */
  MessageReader(DispatchFunc) noexcept;

  /// @brief Main loop - reades messages and sens them to dispatcher
  void Loop() const noexcept;

private:
  MessageDispatcher dispatcher_;
  const std::string kStrAction = "action:";
  const std::string kStrObjects = "_objects:";
};

#endif // MESSAGE_READER_HPP
