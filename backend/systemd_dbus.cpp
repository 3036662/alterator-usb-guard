#include "systemd_dbus.hpp"
#include "log.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <exception>
#include <iostream>
#include <thread>

namespace dbus_bindings {

using guard::utils::Log;

Systemd::Systemd() noexcept : connection_{nullptr} { ConnectToSystemDbus(); }

std::optional<bool>
Systemd::IsUnitEnabled(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  std::string result;
  try {
    auto proxy = CreateProxyToSystemd(kObjectPath);
    auto method =
        proxy->createMethodCall(kSystemdInterfaceManager, "GetUnitFileState");
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

std::optional<bool>
Systemd::IsUnitActive(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  std::string result;
  try {
    // get unit dbus path
    auto proxy = CreateProxyToSystemd(kObjectPath);
    auto method = proxy->createMethodCall(kSystemdInterfaceManager, "LoadUnit");
    method << unit_name;
    auto reply = proxy->callMethod(method);
    sdbus::ObjectPath unit_path;
    reply >> unit_path;

    // get unit Active State
    // proxy to unit interface dbus
    auto proxy_unit = CreateProxyToSystemd(unit_path);
    auto active_state = proxy_unit->getProperty("ActiveState")
                            .onInterface(kSystemdInterfaceUnit);
    result = active_state.get<std::string>();
  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't check if " << unit_name << " unit is active";
  }
  return result.empty()
             ? std::nullopt
             : std::optional<bool>(boost::starts_with(result, "active"));
}

std::optional<bool> Systemd::StartUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto proxy = CreateProxyToSystemd(kObjectPath);
    auto method =
        proxy->createMethodCall(kSystemdInterfaceManager, "StartUnit");
    method << unit_name << "replace";
    auto reply = proxy->callMethod(method);
    auto isActive = IsUnitActive(unit_name);
    if (isActive && isActive.value()) {
      return true;
    }
    for (int i = 0; i < 10; ++i) {
      Log::Info() << "Waiting for systemd starts the sevice ...";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      isActive = IsUnitActive(unit_name);
      if (isActive.has_value() && isActive.value())
        return true;
    }

  } catch (const std::exception &ex) {
    Log::Error() << "Can't restart " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

std::optional<bool> Systemd::EnableUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;

  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::vector<std::string> arr_unit_names{unit_name};
    auto proxy = CreateProxyToSystemd(kObjectPath);
    auto method =
        proxy->createMethodCall(kSystemdInterfaceManager, "EnableUnitFiles");
    method << arr_unit_names << false << true;
    auto reply = proxy->callMethod(method);
    auto isEnabled = IsUnitEnabled(unit_name);
    if (isEnabled && isEnabled.value()) {
      return true;
    }
    for (int i = 0; i < 10; ++i) {
      Log::Info() << "Waiting for systemd starts the sevice ...";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      isEnabled = IsUnitEnabled(unit_name);
      if (isEnabled.has_value() && isEnabled.value())
        return true;
    }

  } catch (const std::exception &ex) {
    Log::Error() << "Can't enable " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

std::optional<bool>
Systemd::DisableUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::vector<std::string> arr_unit_names{unit_name};
    auto proxy = CreateProxyToSystemd(kObjectPath);
    auto method =
        proxy->createMethodCall(kSystemdInterfaceManager, "DisableUnitFiles");
    method << arr_unit_names << false;
    auto reply = proxy->callMethod(method);
    auto isEnabled = IsUnitEnabled(unit_name);
    if (isEnabled && !isEnabled.value()) {
      return true;
    }
    for (int i = 0; i < 10; ++i) {
      Log::Info() << "Waiting for systemd starts the sevice ...";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      isEnabled = IsUnitEnabled(unit_name);
      if (isEnabled.has_value() && !isEnabled.value())
        return true;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't disable " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

std::optional<bool>
Systemd::RestartUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;

  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto proxy = CreateProxyToSystemd(kObjectPath);
    auto method =
        proxy->createMethodCall(kSystemdInterfaceManager, "RestartUnit");
    method << unit_name << "replace";
    auto reply = proxy->callMethod(method);
    auto isActive = IsUnitActive(unit_name);
    if (isActive && isActive.value()) {
      return true;
    }
    for (int i = 0; i < 10; ++i) {
      Log::Info() << "Waiting for systemd restarts the sevice ...";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      isActive = IsUnitActive(unit_name);
      if (isActive.has_value() && isActive.value())
        return true;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't restart " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

std::optional<bool> Systemd::StopUnit(const std::string &unit_name) noexcept {
  if (!Health())
    return std::nullopt;
  try {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // if a unit is already stopped
    auto isActive = IsUnitActive(unit_name);
    if (isActive && !isActive.value()) {
      return true;
    }
    auto proxy = CreateProxyToSystemd(kObjectPath);
    auto method = proxy->createMethodCall(kSystemdInterfaceManager, "StopUnit");
    method << unit_name << "replace";
    auto reply = proxy->callMethod(method);
    isActive = IsUnitActive(unit_name);
    if (isActive && !isActive.value()) {
      return true;
    }
    for (int i = 0; i < 10; ++i) {
      Log::Info() << "Waiting for systemd stops the sevice ...";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      isActive = IsUnitActive(unit_name);
      if (isActive.has_value() && !isActive.value())
        return true;
    }
  } catch (const std::exception &ex) {
    Log::Error() << "Can't stop " << unit_name << " unit is active";
    Log::Error() << ex.what();
  }
  return std::nullopt;
}

/*******************    Private *****************************/

void Systemd::ConnectToSystemDbus() noexcept {
  try {
    connection_ = sdbus::createSystemBusConnection();
  } catch (const sdbus::Error &ex) {
    Log::Error() << "Can't create connection to SDbus";
  }
}

bool Systemd::Health() noexcept {
  if (!connection_)
    ConnectToSystemDbus();
  return static_cast<bool>(connection_);
}

std::unique_ptr<sdbus::IProxy>
Systemd::CreateProxyToSystemd(const std::string &path) {
  return sdbus::createProxy(*connection_, kDestinationName, path);
}

} // namespace dbus_bindings