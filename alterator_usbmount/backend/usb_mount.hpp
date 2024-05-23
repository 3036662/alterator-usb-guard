#pragma once
#include "active_device.hpp"
#include <memory>
#include <sdbus-c++/sdbus-c++.h>

namespace alterator::usbmount {

struct DbusOneParam {
  std::string method;
  std::string param;
};

class UsbMount {
public:
  UsbMount() noexcept;

  std::vector<ActiveDevice> ListDevices() const noexcept;
  std::string getRulesJson() const noexcept;
  std::string GetUsersGroups() const noexcept;
  std::string SaveRules(const std::string &) const noexcept;
  bool Health() const noexcept;
  bool Run() noexcept;
  bool Stop() noexcept;

private:
  const std::string kDest = "ru.alterator.usbd";
  const std::string kObjectPath = "/ru/alterator/altusbd";
  const std::string kInterfaceName = "ru.alterator.Usbd";
  const std::string kServiceUnitName = "altusbd.service";

  std::string GetStringNoParams(const std::string &method_name) const noexcept;
  std::string GetStringResponse(const DbusOneParam &) const noexcept;
  std::unique_ptr<sdbus::IProxy> dbus_proxy_;
};

} // namespace alterator::usbmount
