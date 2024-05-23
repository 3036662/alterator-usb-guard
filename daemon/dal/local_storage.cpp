#include "local_storage.hpp"
#include "mount_points.hpp"
#include <memory>
#include <mutex>

namespace usbmount::dal {

std::shared_ptr<LocalStorage> LocalStorage::p_instance_{nullptr};
std::mutex LocalStorage::mutex_;

std::shared_ptr<LocalStorage> LocalStorage::GetStorage() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!p_instance_) {
    p_instance_.reset(new LocalStorage());
  }
  return p_instance_;
}

LocalStorage::LocalStorage()
    : permissions("/var/lib/alt-usb-mount/permissions.json"),
      mount_points("/var/lib/alt-usb-mount/mount_points.json") {}
} // namespace usbmount::dal