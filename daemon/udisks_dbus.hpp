#pragma once
#include "usb_udev_device.hpp"
#include <memory.h>
#include <sdbus-c++/sdbus-c++.h>

namespace usbmount {

class UdisksDbus {
public:
  UdisksDbus() = delete;
  explicit UdisksDbus(std::unique_ptr<sdbus::IConnection> &conn);

  bool ProcessDevice(const UsbUdevDevice &dev);

private:
  std::unique_ptr<sdbus::IConnection> &dbus_;
  std::unique_ptr<sdbus::IProxy> proxy_udisks_;
};

} // namespace usbmount