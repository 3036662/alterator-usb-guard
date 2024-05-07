#include "guard.hpp"
#include "lisp_message.hpp"
#include "message_dispatcher.hpp"

namespace guard {

class DispatcherImpl {
public:
  explicit DispatcherImpl(Guard &guard);

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

  static constexpr const char *kMessBeg = "(";
  static constexpr const char *kMessEnd = ")";

  Guard &guard_;
};

} // namespace guard
