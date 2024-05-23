#pragma once
#include <libudev.h>
#include <memory>
#include <string>

namespace usbmount {

enum class Action { kRemove, kAdd, kChange, kNoAction, kUndefined };
/// a  custom deleter for shared_ptr<udev_device>
void UdevDeviceFree(udev_device *dev) noexcept;

/// A structure to store Device constructor parameters - dev_path and action
struct DevParams {
  std::string dev_path;
  std::string action;
};

using UniquePtrUdevDeviceStruct =
    std::unique_ptr<udev_device, decltype(&UdevDeviceFree)>;

class UsbUdevDevice {
public:
  /// construct with Udev device object
  explicit UsbUdevDevice(UniquePtrUdevDeviceStruct &&);

  /**
   * @brief Construct a new Usb Udev Device object with path and action
   * @param params DevParams struct {dev_path,action}
   * @throws std::runtime_error
   */
  explicit UsbUdevDevice(const DevParams &params);

  std::string toString() const noexcept;

  // getters
  inline Action action() const noexcept { return action_; }
  inline const std::string &subsystem() const noexcept { return subsystem_; }
  inline const std::string &fs_label() const noexcept { return fs_label_; }
  inline const std::string &fs_uid() const noexcept { return uid_; }
  inline const std::string &filesystem() const noexcept { return filesystem_; }
  inline const std::string &dev_type() const noexcept { return dev_type_; }
  inline const std::string &block_name() const noexcept { return block_name_; }
  inline int partition_number() const noexcept { return partitions_number_; }
  inline const std::string &vid() const noexcept { return vid_; }
  inline const std::string &pid() const noexcept { return pid_; }
  inline const std::string &serial() const noexcept { return serial_; }

  void SetAction(const char *p_action);

private:
  /**
   * @brief This function is a workaround for some cases when the Udev has no
   * ID_SERIAL_SHORT value. It traverses the Udev devices tree to find a serial
   * number for a parent device.
   *
   * @param device
   */
  void FindSerial(UniquePtrUdevDeviceStruct &device) noexcept;

  /// @brief find an info about device and fill the member fields
  void getUdevDeviceInfo(UniquePtrUdevDeviceStruct &device);

  /// @brief find an udev device struct by it's block-name.
  UniquePtrUdevDeviceStruct FindUdevDeviceByBlockName() const;

  Action action_ = Action::kUndefined;
  std::string subsystem_;
  std::string block_name_; /// /dev/sdX
  std::string id_bus_;     //  ID_BUS
  std::string fs_label_;   /// ID_FS_LABEL
  std::string uid_;        /// ID_FS_UUID
  std::string filesystem_; /// ID_FS_TYPE
  std::string dev_type_;   /// DEVTYPE e.g "partition"
  int partitions_number_ = 0;
  std::string vid_;
  std::string pid_;
  std::string serial_;
};

} // namespace usbmount