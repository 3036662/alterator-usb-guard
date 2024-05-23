#pragma once
#include "device_permissions.hpp"
#include "dto.hpp"
#include "mount_points.hpp"
#include "table.hpp"
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace usbmount::dal {

/**
 * @brief Local storage holds data about known devices and mountpoints.
 *
 */

class LocalStorage {
public:
  LocalStorage(const LocalStorage &) = delete;
  LocalStorage(LocalStorage &&) = delete;
  LocalStorage &operator=(const LocalStorage &) = delete;
  LocalStorage &operator=(LocalStorage &&) = delete;

  /**
   * @brief Get the Storage object
   * @return LocalStorage
   */
  static std::shared_ptr<LocalStorage> GetStorage();

  DevicePermissions permissions;
  Mountpoints mount_points;

private:
  LocalStorage();

  static std::shared_ptr<LocalStorage> p_instance_;
  static std::mutex mutex_;
};

} // namespace usbmount::dal

// dao.mountpoints().add();