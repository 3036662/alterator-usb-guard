#pragma once
#include "dal/local_storage.hpp"
#include "udev_monitor.hpp"
#include <boost/json/array.hpp>
#include <memory>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/sdbus-c++.h>
#include <spdlog/logger.h>
#include <string>

namespace usbmount {

class DbusMethods {
public:
  DbusMethods(const DbusMethods &) = delete;
  DbusMethods(DbusMethods &&) = delete;
  DbusMethods &operator=(const DbusMethods &) = delete;
  DbusMethods &&operator=(DbusMethods &&) = delete;
  DbusMethods() = delete;
  explicit DbusMethods(std::shared_ptr<UdevMonitor> udev_monitor,
                       std::shared_ptr<spdlog::logger> logger);

  void Run();

private:
  /** @brief Health method for DBus returns "OK" to caller */
  static void Health(const sdbus::MethodCall &);

  void CanAnotherUserUnmount(sdbus::MethodCall);
  void CanUserMount(sdbus::MethodCall);
  void ListActiveDevices(const sdbus::MethodCall &);
  void ListActiveRules(const sdbus::MethodCall &);
  void GetSystemUsersAndGroups(const sdbus::MethodCall &);
  void SaveRules(sdbus::MethodCall);

  void UpdateRules(const boost::json::array &arr_updated);
  void CreateRules(const boost::json::array &arr_created);

  const std::string service_name = "ru.alterator.usbd";
  const std::string object_path = "/ru/alterator/altusbd";
  const std::string interface_name = "ru.alterator.Usbd";
  std::unique_ptr<sdbus::IConnection> connection_;
  std::unique_ptr<sdbus::IObject> dbus_object_ptr;
  std::shared_ptr<spdlog::logger> logger_;
  std::shared_ptr<dal::LocalStorage> dbase_;
  std::shared_ptr<UdevMonitor> udev_monitor_;
};

} // namespace usbmount