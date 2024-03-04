#include "json_rule.hpp"
#include <sstream>

namespace guard::utils::json {

namespace json = boost::json;

JsonRule::JsonRule(const boost::json::object *ptr_obj) {
  // target
  if (!ptr_obj->contains("target")) {
    throw std::logic_error("Rule target is mandatory");
  }
  target_ = ptr_obj->at("target").as_string().c_str();

  // fields
  if (!ptr_obj->contains("fields_arr") ||
      !ptr_obj->at("fields_arr").is_array() ||
      ptr_obj->at("fields_arr").if_array()->empty()) {
    throw std::logic_error("Can't find any fields for a rule");
  }
  const json::array *ptr_fields = ptr_obj->at("fields_arr").if_array();

  // for each field in array
  for (const auto &field_value : *ptr_fields) {
    if (!field_value.is_object()) {
      throw std::logic_error("Error parsing rule fields");
    }
    const json::object *ptr_field = field_value.if_object();
    if (ptr_field != nullptr)
      ParseOneField(ptr_field);
  }
}

std::string JsonRule::BuildString() const noexcept {
  std::ostringstream string_builder;
  if (!raw_.empty()) {
    string_builder << raw_;
  } else {
    string_builder << target_ << " ";
    if (!vid_.empty())
      string_builder << "id " << vid_ << ":" << pid_ << " ";
    if (!serial_.empty())
      string_builder << "serial " << serial_ << " ";
    if (!device_name_.empty())
      string_builder << "name " << device_name_ << " ";
    if (!hash_.empty())
      string_builder << "hash " << hash_ << " ";
    if (!parent_hash_.empty())
      string_builder << "parent-hash " << parent_hash_ << " ";
    if (!port_.empty())
      string_builder << "via-port " << port_ << " ";
    if (!interface_.empty())
      string_builder << "with-interface " << interface_ << " ";
    if (!connection_.empty())
      string_builder << "with-connect-type " << connection_ << " ";
    if (!condition_.empty())
      string_builder << condition_;
  }
  return string_builder.str();
}

void JsonRule::ParseOneField(const boost::json::object *ptr_field) {
  for (const auto &field_obj : *ptr_field) {
    std::string field =std::string(field_obj.key().cbegin(),field_obj.key().cend());
    std::string value = field_obj.value().as_string().c_str();
    if (value.empty()) {
      throw std::logic_error("Empty value for field " + field);
    }
    // write fields
    if (field == "vid") {
      vid_ = std::move(value);
      continue;
    }
    if (field == "pid") {
      pid_ = std::move(value);
      continue;
    }
    if (field == "hash") {
      hash_ = std::move(value);
      continue;
    }
    if (field == "parent_hash") {
      parent_hash_ = std::move(value);
      continue;
    }
    if (field == "device_name") {
      device_name_ = std::move(value);
      continue;
    }
    if (field == "serial") {
      serial_ = std::move(value);
      continue;
    }
    if (field == "via-port") {
      port_ = std::move(value);
      continue;
    }
    if (field == "with_interface") {
      interface_ = std::move(value);
      continue;
    }
    if (field == "with-connect-type") {
      connection_ = std::move(value);
      continue;
    }
    if (field == "cond") {
      condition_ = std::move(value);
      continue;
    }
    // for a raw rule
    if (field == "raw_rule") {
      raw_ = std::move(value);
      continue;
    }
  }
}

} // namespace guard::utils::json