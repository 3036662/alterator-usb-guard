#include "guard_audit.hpp"
#include "guard.hpp"
#include "log.hpp"
#include "log_reader.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace guard {

using common_utils::Log;
using common_utils::LogReader;

GuardAudit::GuardAudit(AuditType type, const std::string &path)
    : LogReader(path), audit_type_(type), audit_file_path_(path) {
  if (type == AuditType::kUndefined)
    throw std::logic_error("Undefined audit type");
  if (type == AuditType::kFileAudit && path.empty())
    throw std::logic_error("The path to audit file is empty");
  if (type == AuditType::kFileAudit && !std::filesystem::exists(path)) {
    Log::Warning() << "USBGuard audit file doesn't exist";
  }
  if (type == AuditType::kLinuxAudit)
    throw std::logic_error("LinuxAudit is used");
}

std::vector<std::string> GuardAudit::GetByFilter(
    const std::vector<std::string> &filters) const noexcept {
  if (audit_type_ == AuditType::kFileAudit) {
    try {
      return GetFromFile(filters);
    } catch (const std::exception &ex) {
      Log::Error() << ex.what();
      return {};
    }
  }
  return {};
}

} // namespace guard