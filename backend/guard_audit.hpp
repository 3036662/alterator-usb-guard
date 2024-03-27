#pragma once
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
class GuardAudit {
public:
  GuardAudit() = delete;
  GuardAudit(const GuardAudit &) = delete;
  GuardAudit(GuardAudit &&) = delete;
  GuardAudit &operator=(const GuardAudit &) = delete;
  GuardAudit &operator=(GuardAudit &&) = delete;
  explicit GuardAudit(AuditType type, const std::string &path);

  std::vector<std::string> GetAll() const noexcept;
  std::vector<std::string>
  GetByFilter(const std::vector<std::string> &filters) const noexcept;

  std::vector<std::string> GetByPage(const std::vector<std::string> &filters,
                                     uint page_number,
                                     uint pages_size) const noexcept;

private:
  std::vector<std::string>
  GetFromFile(const std::vector<std::string> &filters) const;

  AuditType audit_type_;
  std::string audit_file_path_;
};

} // namespace guard