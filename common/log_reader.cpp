#include "log_reader.hpp"
#include "log.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace common_utils {

namespace fs = std::filesystem;
using common_utils::Log;

LogReader::LogReader(const std::string &fpath) : log_file_path_(fpath) {
  if (log_file_path_.empty())
    throw std::invalid_argument("Empty filepath");
}

vecstring LogReader::GetFromFile(const vecstring &filters) const {
  vecstring res;
  if (log_file_path_.empty())
    throw std::logic_error("An empty path to file");
  if (!fs::exists(log_file_path_))
    throw std::logic_error("File doesn't exist");
  std::ifstream file(log_file_path_);
  if (!file.is_open())
    throw std::runtime_error("Can't open file");
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

vecstring LogReader::GetByFilter(const vecstring &filters) const noexcept {
  try {
    return GetFromFile(filters);
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
    return {};
  }
  return {};
}

vecstring LogReader::GetAll() const noexcept { return GetByFilter({}); }

PageData LogReader::GetByPage(const vecstring &filters, uint page_number,
                               uint pages_size) const noexcept {
  PageData data;
  data.curr_page=page_number;
  data.pages_number=0;
  try {
    std::vector<std::string> all_lines = GetFromFile(filters);
    if (pages_size>0)
      data.pages_number=static_cast<uint>(all_lines.size()/pages_size);
    int lines_size = static_cast<int>(all_lines.size());
    int index_last = lines_size - static_cast<int>(page_number * pages_size);
    int index_first =
        lines_size - static_cast<int>((page_number + 1) * pages_size);
    if (index_first < 0)
      index_first = 0;
    if (index_last > lines_size)
      index_last = lines_size;
    if (index_first >= index_last)
      return data;
    for (size_t i = static_cast<size_t>(index_first);
         i < static_cast<size_t>(index_last); ++i) {
      data.data.emplace_back(std::move(all_lines.at(i)));
    }
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
  }
  return data;
}

} // namespace usbmount