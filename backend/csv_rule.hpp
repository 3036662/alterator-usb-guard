#pragma once
#include <string>

#include "rapidcsv.h"

namespace guard::utils::csv {

class CsvRule {
   public:
    explicit CsvRule(const rapidcsv::Document &doc, size_t index);
    std::string BuildString() const noexcept;

   private:
    std::string target_;
    std::string interface_;
    std::string vidpid_;
    std::string hash_;
};

}  // namespace guard::utils::csv