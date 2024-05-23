#pragma once
#include <string>
#include <sys/types.h>
#include <vector>

namespace common_utils {

using vecstring = std::vector<std::string>;

struct PageData{
    uint curr_page=0;
    uint pages_number=0;
    vecstring data;
};

class LogReader {
public:
  LogReader() = delete;

  /**
   * @brief Construct a new Log Reader object
   * @param fpath
   * @throws std::invalid argument if empty path
   */
  explicit LogReader(const std::string &fpath);

  vecstring GetAll() const noexcept;
  vecstring GetByFilter(const vecstring &filters) const noexcept;
  PageData GetByPage(const vecstring &filters, uint page_number,
                      uint pages_size) const noexcept; 
protected:
  /**
   * @brief Get the From File object
   *
   * @param filters
   * @return vecstring
   * @throws std::logic_error if file doesn't exist or std::runtime_error if
   * can't open
   */
  vecstring GetFromFile(const vecstring &filters) const;                   

private:

  std::string log_file_path_;
};

} // namespace usbmount