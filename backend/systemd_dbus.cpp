#include "systemd_dbus.hpp"
#include "log.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <thread>

namespace dbus_bindings {

using guard::utils::Log;
/******************************************************************************/

Systemd::Systemd() noexcept : connection{nullptr} { ConnectToSystemDbus(); }

std::optional<bool>
Systemd::IsUnitEnabled(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  std::string result;
  try {
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "GetUnitFileState");
    method << unit_name;
    auto reply = proxy->callMethod(method);
    reply >> result;
  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't check if " << unit_name << " unit is enabled";
  }
  return result.empty()
             ? std::nullopt
             : std::optional<bool>(boost::contains(result, "enabled"));
}

/******************************************************************************/

std::optional<bool>
Systemd::IsUnitActive(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  std::string result;
  try {
    // get unit dbus path
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "LoadUnit");
    method << unit_name;
    auto reply = proxy->callMethod(method);
    sdbus::ObjectPath unit_path;
    reply >> unit_path;

    // get unit Active State
    // proxy to unit interface dbus
    auto proxy_unit = CreateProxyToSystemd(unit_path);
    auto active_state = proxy_unit->getProperty("ActiveState")
                            .onInterface(systemd_interface_unit);
    result = active_state.get<std::string>();
  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't check if " << unit_name << " unit is active";
  }
  return result.empty()
             ? std::nullopt
             : std::optional<bool>(boost::starts_with(result, "active"));
}

/******************************************************************************/

std::optional<bool> Systemd::StartUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "StartUnit");
    method << unit_name << "replace";
    auto reply = proxy->callMethod(method);
    auto isActive = IsUnitActive(unit_name);
    if (isActive && isActive.value()) {
      return true;
    } else {
      for (int i = 0; i < 10; ++i) {
        Log::Info() << "Waiting for systemd starts the sevice ...";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isActive = IsUnitActive(unit_name);
        if (isActive.has_value() && isActive.value())
          return true;
      }
    }

  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't restart " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}
/******************************************************************************/

std::optional<bool> Systemd::EnableUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;

  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<std::string> arr_unit_names{unit_name};
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "EnableUnitFiles");
    method << arr_unit_names << false << true;
    auto reply = proxy->callMethod(method);
    auto isEnabled = IsUnitEnabled(unit_name);
    if (isEnabled && isEnabled.value()) {
      return true;
    } else {
      for (int i = 0; i < 10; ++i) {
        Log::Info() << "Waiting for systemd starts the sevice ...";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isEnabled = IsUnitEnabled(unit_name);
        if (isEnabled.has_value() && isEnabled.value())
          return true;
      }
    }

  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't enable " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

/******************************************************************************/

std::optional<bool>
Systemd::DisableUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<std::string> arr_unit_names{unit_name};
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "DisableUnitFiles");
    method << arr_unit_names << false;
    auto reply = proxy->callMethod(method);
    auto isEnabled = IsUnitEnabled(unit_name);
    if (isEnabled && !isEnabled.value()) {
      return true;
    } else {
      for (int i = 0; i < 10; ++i) {
        Log::Info() << "Waiting for systemd starts the sevice ...";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isEnabled = IsUnitEnabled(unit_name);
        if (isEnabled.has_value() && !isEnabled.value())
          return true;
      }
    }

  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't disable " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

/******************************************************************************/
std::optional<bool>
Systemd::RestartUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;

  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "RestartUnit");
    method << unit_name << "replace";
    auto reply = proxy->callMethod(method);
    auto isActive = IsUnitActive(unit_name);
    if (isActive && isActive.value()) {
      return true;
    } else {
      for (int i = 0; i < 10; ++i) {
        Log::Info() << "Waiting for systemd restarts the sevice ...";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isActive = IsUnitActive(unit_name);
        if (isActive.has_value() && isActive.value())
          return true;
      }
    }

  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't restart " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

/******************************************************************************/
std::optional<bool> Systemd::StopUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;

  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // if a unit is already stopped
    auto isActive = IsUnitActive(unit_name);
    if (isActive && !isActive.value()) {
      return true;
    }
    auto proxy = CreateProxyToSystemd(objectPath);
    auto method =
        proxy->createMethodCall(systemd_interface_manager, "StopUnit");
    method << unit_name << "replace";
    auto reply = proxy->callMethod(method);
    isActive = IsUnitActive(unit_name);
    if (isActive && !isActive.value()) {
      return true;
    } else {
      for (int i = 0; i < 10; ++i) {
        Log::Info() << "Waiting for systemd stops the sevice ...";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        isActive = IsUnitActive(unit_name);
        if (isActive.has_value() && !isActive.value())
          return true;
      }
    }
  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't stop " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

/*******************    Private *****************************/

void Systemd::ConnectToSystemDbus() noexcept {
  try {
    connection = sdbus::createSystemBusConnection();
  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't create connection to SDbus";
  }
}

/******************************************************************************/

bool Systemd::Health() noexcept {
  if (!connection)
    ConnectToSystemDbus();
  return static_cast<bool>(connection);
}

/******************************************************************************/

std::unique_ptr<sdbus::IProxy>
Systemd::CreateProxyToSystemd(const std::string &path) {
  return sdbus::createProxy(*connection, destinationName, path);
}

} // namespace dbus_bindings