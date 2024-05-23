#include "lisp_message.hpp"

LispMessage::LispMessage() {}

LispMessage::LispMessage(
    const MsgAction &act, const MsgObject &obj,
    const std::unordered_map<std::string, std::string> &prms) noexcept
    : action(act.val), objects(obj.val), params(prms) {}

std::ostream &operator<<(std::ostream &ostream, const LispMessage &mes) {
  ostream << "Action: " << mes.action << "\n"
          << "Objects: " << mes.objects << "\n";
  for (auto it = mes.params.cbegin(); it != mes.params.end(); ++it) {
    ostream << it->first << " : " << it->second << "\n";
  }
  return ostream;
}
