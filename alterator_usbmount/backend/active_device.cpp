#include "active_device.hpp"
#include "types.hpp"
#include <boost/json/object.hpp>
#include <stdexcept>
#include <string>

namespace alterator::usbmount {

ActiveDevice::ActiveDevice(const json::object &obj) {
  if (!obj.contains("device") || !obj.contains("vid") || !obj.contains("pid") ||
      !obj.contains("serial") || !obj.contains("mount") ||
      !obj.contains("status") || !obj.contains("fs"))
    throw std::invalid_argument("Error parsing Json object");
  block = obj.at("device").as_string().c_str();
  fs = obj.at("fs").as_string().c_str();
  vid = obj.at("vid").as_string().c_str();
  pid = obj.at("pid").as_string().c_str();
  serial = obj.at("serial").as_string().c_str();
  mount_point = obj.at("mount").as_string().c_str();
  status = obj.at("status").as_string().c_str();
}

vecPairs ActiveDevice::SerializeForLisp() const noexcept {
  vecPairs res;
  res.emplace_back("name", std::to_string(index));
  res.emplace_back("lbl_block", block);
  res.emplace_back("lbl_fs", fs);
  res.emplace_back("lbl_vid", vid);
  res.emplace_back("lbl_pid", pid);
  res.emplace_back("lbl_serial", serial);
  res.emplace_back("lbl_mount", mount_point);
  res.emplace_back("lbl_status", status);
  return res;
}

} // namespace alterator::usbmount