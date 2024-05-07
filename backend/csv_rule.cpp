#include "csv_rule.hpp"
#include "common_utils.hpp"
#include "log.hpp"
#include <cstddef>
#include <stdexcept>

namespace guard::utils::csv {

CsvRule::CsvRule(rapidcsv::Document &doc, size_t index) {
  std::vector<std::string> row = doc.GetRow<std::string>(index);
  size_t n_cols = row.size();
  if (row.size() < 2)
    throw std::logic_error("Empty csv row");
  target_ = std::move(row[0]);
  if (target_.empty())
    throw std::logic_error("Empty rule target");
  if (n_cols >= 2)
    interface_ = row[1];
  if (n_cols >= 3)
    vidpid_ = row[2];
  if (n_cols >= 4)
    hash_ = row[3];
  if (interface_.empty() && vidpid_.empty() && hash_.empty())
    throw std::logic_error("Empty csv row");
}

std::string CsvRule::BuildString() const noexcept {
  std::ostringstream string_builder;
  string_builder << target_ << " ";
  if (!vidpid_.empty())
    string_builder << "id " << vidpid_ << " ";
  if (!hash_.empty())
    string_builder << "hash " << common_utils::QuoteIfNotQuoted(hash_) << " ";
  if (!interface_.empty())
    string_builder << "with-interface " << interface_ << " ";
  return string_builder.str();
}

} // namespace guard::utils::csv