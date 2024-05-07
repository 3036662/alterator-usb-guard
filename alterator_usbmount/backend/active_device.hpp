#pragma once

#include "serializable_for_lisp.hpp"
#include "types.hpp"
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <cstddef>
#include <string>

namespace alterator::usbmount {

namespace json = boost::json;

struct ActiveDevice : SerializableForLisp<ActiveDevice> {
  size_t index = 0;
  std::string block;
  std::string fs;
  std::string vid;
  std::string pid;
  std::string serial;
  std::string mount_point;
  std::string status;

  vecPairs SerializeForLisp() const noexcept;

  /**
   * @brief Construct a new Active Device object
   * @param obj
   * @throws std::invalid_argument
   */
  explicit ActiveDevice(const json::object &obj);
};

} // namespace alterator::usbmount