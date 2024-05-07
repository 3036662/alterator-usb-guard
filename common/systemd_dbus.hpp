#pragma once

#include <memory>
#include <optional>
#include <sdbus-c++/sdbus-c++.h>

namespace dbus_bindings {

/**
 * @class Systemd
 * @brief A binding for some usefull systemd DBus functions
 *
 */
class Systemd {
public:
  Systemd() noexcept; /// constructor - creates connection to DBus

  /**
   * @brief Check whether unit.service is enabled
   * @param unit_name File.service (string)
   * @return True if unit is enabled,False if not, nothing if error(optional)
   * */
  std::optional<bool> IsUnitEnabled(const std::string &unit_name) noexcept;
  /**
   * @brief Check whether unit.service is active
   * @param unit_name File.service active
   * @returnTrue if unit is enabled,False if not, nothing if error (optional)
   * */
  std::optional<bool> IsUnitActive(const std::string &unit_name) noexcept;
  /// @brief is unit is running now
  std::optional<bool> StartUnit(const std::string &unit_name) noexcept;
  /// @brief restart
  /// @return true if succeded
  std::optional<bool> RestartUnit(const std::string &unit_name) noexcept;
  /// @brief aka systemctl stop
  /// @return true if succeded
  std::optional<bool> StopUnit(const std::string &unit_name) noexcept;
  /// @brief aka systemctl enable
  /// @return true if succeded
  std::optional<bool> EnableUnit(const std::string &unit_name) noexcept;
  /// @brief restart
  /// @return true if succeded
  std::optional<bool> DisableUnit(const std::string &unit_name) noexcept;

private:
  void ConnectToSystemDbus() noexcept;
  bool Health() noexcept; /// check if normally connectted
  /**
   * @brief Create proxy for systemd particular path
   * @param path String interface (for inst. /org/freedesktop/systemd1)'
   * @return Unique ptr to IProxy object
   * */
  std::unique_ptr<sdbus::IProxy> CreateProxyToSystemd(const std::string &path);

  const std::string kDestinationName = "org.freedesktop.systemd1";
  const std::string kObjectPath = "/org/freedesktop/systemd1";
  const std::string kSystemdInterfaceManager =
      "org.freedesktop.systemd1.Manager";
  const std::string kSystemdInterfaceUnit = "org.freedesktop.systemd1.Unit";
  std::unique_ptr<sdbus::IConnection> connection_;
};

} // namespace dbus_bindings