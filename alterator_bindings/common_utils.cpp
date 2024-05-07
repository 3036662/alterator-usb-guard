#include "common_utils.hpp"
#include "log.hpp"
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace common_utils {

std::string UnUtf8(const std::string &str) noexcept {
  std::string res;
  std::runtime_error ex_bad_string("Invalid characters in string");
  try {
    const uint kMaxIter = 1'000'000;
    uint iter_counter = 0;
    size_t ind = 0;
    while (ind < str.size()) {
      ++iter_counter;
      if (iter_counter > kMaxIter)
        throw std::runtime_error("Maximal number of iterations was reached");
      // 1 byte char - skip quotes
      if ((str.at(ind) & 0b10000000) == 0) {
        if (str.at(ind) != '\"' && str.at(ind) != '\'')
          res += str.at(ind);
        ++ind;
        continue;
      }
      // 2 bytes - skip
      if ((str.at(ind) & 0b11100000) == 0b11000000 && ind < str.size() - 1) {
        ind += 2;
        continue;
      }
      // 3 bytes - skip
      if ((str.at(ind) & 0b11110000) == 0b11100000 && ind < str.size() - 2) {
        ind += 3;
        continue;
      }
      // 4 bytes - skip
      if ((str.at(ind) & 0b11111000) == 0b11110000) {
        ind += 4;
        continue;
      }
      throw std::runtime_error("Bad utf-8 string");
    }
  } catch (const std::exception &ex) {
    Log::Error() << ex.what();
  }
  return res;
}

std::string WrapWithQuotes(const std::string &str) noexcept {
  std::string res;
  res += "\"";
  res += str;
  res += "\"";
  return res;
}

std::string UnQuote(const std::string &str) noexcept {
  std::string res = str;
  try {
    if (str.size() >= 2 && str[0] == '\"' && str.back() == '\"') {
      res = std::string(str, 1, str.size() - 2);
    }
  } catch (const std::exception &ex) {
    Log::Debug() << "Error Unquoting string(UnQoute)";
    return str;
  }
  return res;
}

std::string QuoteIfNotQuoted(const std::string &str) noexcept {
  if (str.empty())
    return "\"\"";
  if (str[0] != '\"' || str.back() != '\"')
    return WrapWithQuotes(str);
  return str;
}

std::optional<uint32_t> StrToUint(const std::string &str) noexcept {
  uint32_t res = 0;
  try {
    if (!str.empty() && str[0] == '-')
      throw std::exception();
    size_t pos = 0;
    res = static_cast<uint32_t>(std::stoul(str, &pos, 10));
    if (pos != str.size())
      throw std::exception();
  } catch (std::exception &e) {
    std::cerr << "Error string to number conversion";
    std::cerr << e.what();
    return std::nullopt;
  }
  return res;
}

std::vector<std::string> FindAllFilesInDirRecursive(
    const std::pair<std::string, std::string> &path_ext) noexcept {
  // TODO think about enabling symlinks support
  namespace fs = std::filesystem;
  const int max_depth = 30;
  std::vector<std::string> res;
  fs::path fs_path(path_ext.first);
  if (fs::exists(fs_path)) {
    for (auto it_entry = fs::recursive_directory_iterator(fs_path);
         it_entry != fs::recursive_directory_iterator(); ++it_entry) {
      if (it_entry.depth() > max_depth)
        break;
      if (it_entry->is_regular_file() &&
          it_entry->path().extension().string() == path_ext.second) {
        res.emplace_back(it_entry->path().string());
      }
    }
  }
  return res;
}

std::string ToLisp(const std::pair<std::string, std::string> &data) noexcept {
  std::string res;
  // ignore name - use olny value
  res += "(";
  res += WrapWithQuotes(data.second);
  res += ")";
  return res;
}

std::string EscapeQuotes(const std::string &str) noexcept {
  std::string res;
  for (auto it = str.cbegin(); it != str.cend(); ++it) {
    if (*it == '\"' && res.back() != '\\') {
      res.push_back('\\');
    }
    res.push_back(*it);
  }
  return res;
}

std::string EscapeAll(const std::string &str) noexcept {
  std::string res;
  for (auto it = str.cbegin(); it != str.cend(); ++it) {
    if (*it == '\"' || *it == '\\') {
      res.push_back('\\');
    }
    res.push_back(*it);
  }
  return res;
}

std::string HtmlEscape(const std::string& str) noexcept{
  std::string res;
  std::unordered_map<char,std::string> escape_map{
    {'\t',"&#9;"},
    {'\n',"&#10;"},
    {'\"',"&#34;"},
    {'\\',"&#92;"},
    {'\'',"&#39;"}
  };
  for (auto it=str.cbegin();it<str.cend();++it){
    if (escape_map.count(*it)==0){
      res.push_back(*it);
    }
    else {
      try {
        res+=escape_map.at(*it); 
      } catch (const std::exception& ex){
        res+=*it;
      }
    }
  }
  return res;
}

} // namespace common_utils