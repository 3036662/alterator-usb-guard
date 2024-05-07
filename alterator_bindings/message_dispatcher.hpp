#pragma once

#include "lisp_message.hpp"
#include <functional>

using DispatchFunc = std::function<bool(const LispMessage &)>;

/**
 * @class MessageDispatcher
 * @brief Accepts messages (LispMessage) from MessageReader and perfoms
 * appropriate actions
 */
class MessageDispatcher {
public:
  /**
   * @brief Constructor for Message Dispatcher
   * @param function bool(*)(const LispMessage&) as dispatcher implementation
   */
  explicit MessageDispatcher(DispatchFunc) noexcept;

  /**
   * @brief Perfom an appropriate action for msg
   * @param msg LispMessage from MessageReader
   */
  bool Dispatch(const LispMessage &msg) const noexcept;

private:
  const DispatchFunc p_dispatcher_func_;
};
