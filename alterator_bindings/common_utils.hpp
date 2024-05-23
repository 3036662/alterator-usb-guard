#pragma once
#include "serializable_for_lisp.hpp"
#include "types.hpp"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace common_utils {

std::string UnUtf8(const std::string &str) noexcept;

/// @brief Wrap string with esape coutes
/// @param str String to wrap
/// @return New wrapped string
std::string WrapWithQuotes(const std::string &str) noexcept;

/**
 * @brief Unwrap a quoted string (remove quotes)
 */
std::string UnQuote(const std::string &str) noexcept;

/**
 * @brief Wrap a string with quotes if it is not quotted
 */
std::string QuoteIfNotQuoted(const std::string &str) noexcept;

/// @brief Exception-safe string to uint32_t conversion
/// @param id String, containing number
/// @return Numerical value (uint32_t)
std::optional<uint32_t> StrToUint(const std::string &str) noexcept;

/// @brief Recursive search for all files in direcrory
/// @param dir Directory to search in
/// @param ext Extension - which files to search
/// @return Vector of string pathes for all files in directory (recursive)
/// @warning may throw exceptions
std::vector<std::string> FindAllFilesInDirRecursive(
    const std::pair<std::string, std::string> &path_ext) noexcept;

/**
 * @brief Converts vec of string pairs to a LispSring
 * @param vec Vector of string
 * @return String, suitable for sending to Lisp(Alterator)
 * @details  Can be used map html table labels to data values.
 * Returns a lisp strig ("value1" "label2" "value2" ...)
 */
template <typename T>
std::string ToLisp(const SerializableForLisp<T> &obj) noexcept {
  std::string res;
  vecPairs vec{obj.SerializeForLisp()};
  res += "(";
  // ignore firs name, use only value
  auto iter = vec.cbegin();
  if (iter != vec.cend()) {
    res += WrapWithQuotes(iter->second);
    res += " ";
    ++iter;
  }
  // use name:value
  while (iter != vec.cend()) {
    res += WrapWithQuotes(iter->first);
    res += " ";
    res += WrapWithQuotes(iter->second);
    res += " ";
    ++iter;
  }
  res += ")";
  // std::cerr << "result string: " <<std::endl <<res << std::endl;
  return res;
}

/**
 * @brief Constructs a lisp associative list from vector ov string pairs
 * @param obj Any SerializableForLisp object
 * @return Lisp string - ((name "value")(name2 "value2"))
 */
template <typename T>
std::string ToLispAssoc(const SerializableForLisp<T> &obj) noexcept {
  std::string res;
  vecPairs vec{obj.SerializeForLisp()};
  res += '(';
  for (const auto &pair : vec) {
    res += '(';
    res += pair.first;
    res += ' ';
    res += WrapWithQuotes(pair.second);
    res += ')';
  }
  res += ')';
  return res;
}

/**
 * @brief Maps one name:value -> lisp string (name , value) for html tables
 * @param name Html label name
 * @param value Value
 * @details Name parameter will be ignored - table with one column
 * needs only value. Returns a lisp string ("value")
 */
std::string ToLisp(const std::pair<std::string, std::string> &data) noexcept;

/// @brief  Escapes double-quotes with slashes
/// @param str Source string
/// @return Escaped string
std::string EscapeQuotes(const std::string &str) noexcept;

std::string EscapeAll(const std::string &str) noexcept;

std::string HtmlEscape(const std::string& str) noexcept;

/**
 * @brief Utility function for timing
 */
template <class result_t = std::chrono::milliseconds,
          class clock_t = std::chrono::steady_clock,
          class duration_t = std::chrono::milliseconds>
auto since(std::chrono::time_point<clock_t, duration_t> const &start) {
  return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

} // namespace common_utils