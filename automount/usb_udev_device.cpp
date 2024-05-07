#include "usb_udev_device.hpp"
#include <exception>
#include <libudev.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <usbguard/Logger.hpp>

UsbUdevDevice::UsbUdevDevice(const DevParams &params,
                             const std::shared_ptr<spdlog::logger> &logger)
    : logger_(logger), action_(Action::kUndefined),
      block_name_(params.dev_path), partitions_number_(0) {
  if (params.dev_path.empty())
    throw std::runtime_error("Empty device name");
  if (params.action.empty())
    throw std::runtime_error("Empty action");
  if (params.action == "add")
    action_ = Action::kAdd;
  else if (params.action == "remove")
    action_ = Action::kRemove;
  if (action_ == Action::kAdd)
    getUdevDeviceInfo();
}

void UsbUdevDevice::getUdevDeviceInfo() {
  // udev object
  std::unique_ptr<udev, decltype(&udev_unref)> udev{udev_new(), udev_unref};
  if (!udev)
    throw std::runtime_error("Failed to create udev object");
  // udev enumerate
  std::unique_ptr<udev_enumerate, decltype(&udev_enumerate_unref)> enumerate{
      udev_enumerate_new(udev.get()), udev_enumerate_unref};
  if (!enumerate)
    throw std::runtime_error("Failed to create udev enumerate");
  udev_enumerate_add_match_subsystem(enumerate.get(), "block");
  udev_enumerate_scan_devices(enumerate.get());
  udev_list_entry *entry = udev_enumerate_get_list_entry(enumerate.get());
  // find a device
  std::unique_ptr<udev_device, decltype(&udev_device_unref)> ptr_device(
      nullptr, udev_device_unref);

  while (entry != NULL) {
    // logger_->debug("ITERATION LOOP");
    const char *p_path = udev_list_entry_get_name(entry);
    std::unique_ptr<udev_device, decltype(&udev_device_unref)> device(
        udev_device_new_from_syspath(udev.get(), p_path), udev_device_unref);
    if (!device) {
      logger_->error("Can't find device {} in udev", p_path);
      continue;
    }
    const char *devnode = udev_device_get_devnode(device.get());
    if (devnode == NULL)
      continue;
    // a device found
    if (block_name_ == std::string(devnode)) {
      ptr_device = std::move(device);
    }
    entry = udev_list_entry_get_next(entry);
  }
  // device found - get info
  // label
  const char *p_label =
      udev_device_get_property_value(ptr_device.get(), "ID_FS_LABEL");
  if (p_label != NULL)
    fs_label_ = p_label;
  // uid
  const char *p_uid =
      udev_device_get_property_value(ptr_device.get(), "ID_FS_UUID");
  if (p_uid != NULL)
    uid_ = p_uid;
  const char *p_fs =
      udev_device_get_property_value(ptr_device.get(), "ID_FS_TYPE");
  if (p_fs != NULL)
    filesystem_ = p_fs;
  const char *p_devtipe =
      udev_device_get_property_value(ptr_device.get(), "DEVTYPE");
  if (p_devtipe != NULL) {
    dev_type_ = p_devtipe;
  }
  // ID_PART_ENTRY_NUMBER
  const char *p_partition_count =
      udev_device_get_property_value(ptr_device.get(), "ID_PART_ENTRY_NUMBER");
  if (p_partition_count != NULL) {
    partitions_number_ = std::stoi(p_partition_count);
  }
}

std::string UsbUdevDevice::toString() const noexcept {
  std::stringstream res;
  res << "Action: ";
  switch (action_) {
  case Action::kAdd:
    res << "add";
    break;
  case Action::kRemove:
    res << "remove";
    break;
  case Action::kUndefined:
    res << "undefined";
    break;
  }
  res << " block_name: " << block_name_ << " fs_label: " << fs_label_
      << " fs_uid: " << uid_ << " file_system: " << filesystem_
      << " device_type:" << dev_type_ << " partition number "
      << std::to_string(partitions_number_);
  return res.str();
}