#pragma once
#include "log_reader.hpp"
#include <string>
#include <vector>

namespace guard {

/**
 * @brief type of UsbGuard audit : File | Linux (audit framework)
 */
enum class AuditType { kFileAudit, kLinuxAudit, kUndefined };

/**
 * @brief Represents guard logger
 *
 */
class GuardAudit : public common_utils::LogReader {
public:
  GuardAudit() = delete;
  GuardAudit(const GuardAudit &) = delete;
  GuardAudit(GuardAudit &&) = delete;
  GuardAudit &operator=(const GuardAudit &) = delete;
  GuardAudit &operator=(GuardAudit &&) = delete;

  /**
   * @brief Construct a new Guard Audit object
   *
   * @param type type of autdit
   * @param path path to file
   * @throws  std::logic_error
   */
  explicit GuardAudit(AuditType type, const std::string &path);

  std::vector<std::string>
  GetByFilter(const std::vector<std::string> &filters) const noexcept;

private:
  AuditType audit_type_;
  std::string audit_file_path_;
};

} // namespace guard