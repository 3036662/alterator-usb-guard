#include "usb_udev_device.hpp"
#include "utils.hpp"
#include <cstddef>
#include <libudev.h>
#include <sstream>
#include <stdexcept>

namespace usbmount {

void UdevDeviceFree(udev_device *dev) noexcept {
  if (dev != NULL)
    udev_device_unref(dev);
}

UsbUdevDevice::UsbUdevDevice(UniquePtrUdevDeviceStruct &&device) {
  // action
  const char *p_action = udev_device_get_property_value(device.get(), "ACTION");
  SetAction(p_action);
  getUdevDeviceInfo(device);
}

UsbUdevDevice::UsbUdevDevice(const DevParams &params)
    : action_(Action::kUndefined), block_name_(params.dev_path),
      partitions_number_(0) {
  if (params.dev_path.empty())
    throw std::runtime_error("Empty device name");
  if (params.action.empty())
    throw std::runtime_error("Empty action");
  if (params.action == "add")
    action_ = Action::kAdd;
  else if (params.action == "remove")
    action_ = Action::kRemove;
  // Information about the device is not needed for actions other than "add"
  if (action_ != Action::kAdd)
    return;
  auto ptr_device = FindUdevDeviceByBlockName();
  // device found
  if (ptr_device)
    getUdevDeviceInfo(ptr_device);
}

void UsbUdevDevice::getUdevDeviceInfo(
    std::unique_ptr<udev_device, decltype(&UdevDeviceFree)> &device) {
  // subsystem
  const char *p_subsystem =
      udev_device_get_property_value(device.get(), "SUBSYSTEM");
  if (p_subsystem != NULL)
    subsystem_ = p_subsystem;
  if (subsystem_ != "block")
    throw std::logic_error("wrong subsystem");
  // bus
  const char *p_idbus = udev_device_get_property_value(device.get(), "ID_BUS");
  if (p_idbus != NULL)
    id_bus_ = p_idbus;
  if (id_bus_ != "usb" && id_bus_ != "USB")
    throw std::logic_error("Wrong ID_BUS");
  // block name
  const char *p_block = udev_device_get_property_value(device.get(), "DEVNAME");
  if (p_block != NULL)
    block_name_ = p_block;
  if (block_name_.empty())
    throw std::logic_error("Empty block name");
  // fs label
  const char *p_label =
      udev_device_get_property_value(device.get(), "ID_FS_LABEL");
  if (p_label != NULL)
    fs_label_ = p_label;
  // fs uuid
  const char *p_uid =
      udev_device_get_property_value(device.get(), "ID_FS_UUID");
  if (p_uid != NULL)
    uid_ = p_uid;
  // fs type
  const char *p_fs = udev_device_get_property_value(device.get(), "ID_FS_TYPE");
  if (p_fs != NULL)
    filesystem_ = p_fs;
  const char *p_devtipe =
      udev_device_get_property_value(device.get(), "DEVTYPE");
  if (p_devtipe != NULL)
    dev_type_ = p_devtipe;
  const char *p_partition_number =
      udev_device_get_property_value(device.get(), "ID_PART_ENTRY_NUMBER");
  if (p_partition_number != NULL)
    partitions_number_ = std::stoi(p_partition_number);
  const char *p_vid =
      udev_device_get_property_value(device.get(), "ID_VENDOR_ID");
  if (p_vid != NULL)
    vid_ = p_vid;
  const char *p_pid =
      udev_device_get_property_value(device.get(), "ID_MODEL_ID");
  if (p_pid != NULL)
    pid_ = p_pid;
  if (action_ != Action::kRemove)
    FindSerial(device);
}

/*
 * This function is a workaround for some cases when the Udev has no
 * ID_SERIAL_SHORT value. It traverses the Udev devices tree to find a serial
 * number for a parent device.
 */
void UsbUdevDevice::FindSerial(
    std::unique_ptr<udev_device, decltype(&UdevDeviceFree)> &device) noexcept {
  // First, try to find it the usual way.
  const char *p_serial_udev =
      udev_device_get_property_value(device.get(), "ID_SERIAL_SHORT");
  if (p_serial_udev != NULL) {
    serial_ = p_serial_udev;
    return;
  }
  // Then read a system attribute.
  const char *p_serial = udev_device_get_sysattr_value(device.get(), "serial");
  if (p_serial != NULL) {
    serial_ = p_serial;
    return;
  }
  // Finally, traverse a Udev tree (maximum = 3 steps).
  size_t it_counter = 0;
  size_t max_iterations = 3;
  udev_device *parent_device = device.get();
  while (serial_.empty() && it_counter < max_iterations) {
    ++it_counter;
    parent_device = udev_device_get_parent_with_subsystem_devtype(parent_device,
                                                                  "usb", NULL);
    if (parent_device != NULL) {
      const char *p_parent_serial =
          udev_device_get_sysattr_value(parent_device, "serial");
      if (p_parent_serial != NULL)
        serial_ = p_parent_serial;
    } else {
      break;
    }
  }
}

void UsbUdevDevice::SetAction(const char *p_action) {
  if (p_action != NULL) {
    std::string tmp = p_action;
    if (tmp == "add")
      action_ = Action::kAdd;
    else if (tmp == "remove")
      action_ = Action::kRemove;
    else if (tmp == "change")
      action_ = Action::kChange;
    else
      action_ = Action::kNoAction;
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
  case Action::kChange:
    res << "change";
    break;
  case Action::kNoAction:
    res << "no_action";
    break;
  case Action::kUndefined:
    res << "undefined";
    break;
  }
  res << " block_name: " << block_name_ << " fs_label: " << fs_label_
      << " fs_uid: " << uid_ << " file_system: " << filesystem_
      << " device_type:" << dev_type_ << " partition number "
      << std::to_string(partitions_number_) << " vid " << vid_ << " pid "
      << pid_ << " subsystem " << subsystem_ << " serial " << serial_;
  return res.str();
}

UniquePtrUdevDeviceStruct UsbUdevDevice::FindUdevDeviceByBlockName() const {
  using utils::udev::UdevEnumerateFree;
  // udev object
  std::unique_ptr<udev, decltype(&udev_unref)> udev{udev_new(), udev_unref};
  if (!udev)
    throw std::runtime_error("Failed to create udev object");
  // udev enumerate
  std::unique_ptr<udev_enumerate, decltype(&UdevEnumerateFree)> enumerate{
      udev_enumerate_new(udev.get()), UdevEnumerateFree};
  if (!enumerate)
    throw std::runtime_error("Failed to create udev enumerate");
  udev_enumerate_add_match_subsystem(enumerate.get(), "block");
  udev_enumerate_add_match_property(enumerate.get(), "ID_BUS", "usb");
  udev_enumerate_scan_devices(enumerate.get());
  udev_list_entry *entry = udev_enumerate_get_list_entry(enumerate.get());
  while (entry != NULL) {
    const char *p_path = udev_list_entry_get_name(entry);
    if (p_path == NULL)
      continue;
    std::unique_ptr<udev_device, decltype(&UdevDeviceFree)> device(
        udev_device_new_from_syspath(udev.get(), p_path), UdevDeviceFree);
    if (!device) {
      // TODO log error
      continue;
    }
    const char *devnode = udev_device_get_devnode(device.get());
    if (devnode == NULL)
      continue;
    // a device found
    if (block_name_ == std::string(devnode)) {
      // ptr_device = std::move(device);
      return device;
    }
    entry = udev_list_entry_get_next(entry);
  }
  return UniquePtrUdevDeviceStruct(nullptr, UdevDeviceFree);
}

} // namespace usbmount