#include "guard_audit.hpp"
#include "guard.hpp"
#include "log.hpp"
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

using guard::utils::Log;

GuardAudit::GuardAudit(AuditType type, const std::string &path)
    : audit_type_(type), audit_file_path_(path) {
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

std::vector<std::string> GuardAudit::GetAll() const noexcept {
  return GetByFilter({});
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

std::vector<std::string>
GuardAudit::GetFromFile(const std::vector<std::string> &filters) const {
  std::vector<std::string> res;
  if (audit_file_path_.empty())
    throw std::logic_error("Empty path to audit file");
  if (!std::filesystem::exists(audit_file_path_))
    throw std::logic_error("Audit file doesn't exist");
  std::ifstream file(audit_file_path_);
  if (!file.is_open()) {
    Log::Error() << "Can't open " << audit_file_path_;
    throw std::runtime_error("Can't open audit file");
  }
  std::string line;
  size_t line_number = 0;
  while (std::getline(file, line)) {
    ++line_number;
    if (filters.empty() || std::any_of(filters.cbegin(), filters.cend(),
                                       [&line](const std::string &filter) {
                                         return boost::contains(line, filter);
                                       })) {
      std::stringstream log_entry;
      log_entry << "[" << std::to_string(line_number) << "] " << line;
      res.emplace_back(log_entry.str());
    }
    line.clear();
  }
  file.close();
  return res;
}

std::vector<std::string>
GuardAudit::GetByPage(const std::vector<std::string> &filters, uint page_number,
                      uint pages_size) const noexcept {
  std::vector<std::string> res;
  try {
    std::vector<std::string> all_lines = GetFromFile(filters);
    int lines_size = static_cast<int>(all_lines.size());
    int index_last = lines_size - static_cast<int>(page_number * pages_size);
    int index_first =
        lines_size - static_cast<int>((page_number + 1) * pages_size);
    if (index_first < 0)
      index_first = 0;
    if (index_last > lines_size)
      index_last = lines_size;
    if (index_first >= index_last)
      return res;
    for (size_t i = static_cast<size_t>(index_first);
         i < static_cast<size_t>(index_last); ++i) {
      res.emplace_back(std::move(all_lines.at(i)));
    }
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
  }
  return res;
}

} // namespace guard