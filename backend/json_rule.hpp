#pragma once
#include <boost/json.hpp>
#include <string>

namespace guard::json {

class JsonRule {
public:
  explicit JsonRule(const boost::json::object *ptr_obj);
  std::string BuildString() const noexcept;

private:
  std::string target_;
  std::string vid_;
  std::string pid_;
  std::string hash_;
  std::string parent_hash_;
  std::string device_name_;
  std::string serial_;
  std::string port_;
  std::string interface_;
  std::string connection_;
  std::string condition_;
  std::string raw_;
  void ParseOneField(const boost::json::object *ptr_field);
};

} // namespace guard::json