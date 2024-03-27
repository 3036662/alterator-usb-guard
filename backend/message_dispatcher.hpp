#pragma once

#include "guard.hpp"
#include "lisp_message.hpp"
#include <string>

/**
 * @class MessageDispatcher
 * @brief Accepts messages (LispMessage) from MessageReader and perfoms
 * appropriate actions
 */
class MessageDispatcher {
public:
  /**
   * @brief Constructor for Message Dispatcher
   * @param guard The Guard object
   */
  explicit MessageDispatcher(guard::Guard &guard) noexcept;
  /**
   * @brief Perfom an appropriate action for msg
   * @param msg LispMessage from MessageReader
   */
  bool Dispatch(const LispMessage &msg) const noexcept;

private:
  bool SaveChangeRules(const LispMessage &msg, bool apply_rules) const noexcept;
  bool ListUsbGuardRules(const LispMessage &msg) const noexcept;
  bool ListUsbDevices() const noexcept;
  bool AllowDevice(const LispMessage &msg) const noexcept;
  bool BlockDevice(const LispMessage &msg) const noexcept;
  bool CheckConfig() const noexcept;
  bool ReadUsbGuardLogs(const LispMessage &msg) const noexcept;
  static bool UploadRulesFile(const LispMessage &msg) noexcept;

  guard::Guard &guard_;
  const std::string kMessBeg = "(";
  const std::string kMessEnd = ")";
};
