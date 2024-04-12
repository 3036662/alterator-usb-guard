#include "udev_monitor.hpp"
#include "custom_mount.hpp"
#include "dal/dto.hpp"
#include "dal/local_storage.hpp"
#include "usb_udev_device.hpp"
#include "utils.hpp"
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <future>
#include <iostream>
#include <libudev.h>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace usbmount {

UdevMonitor::UdevMonitor(std::shared_ptr<spdlog::logger> &logger)
    : logger_(logger), future_obj_(stop_signal_.get_future()),
      udev_(udev_new(), udev_unref),
      monitor_(udev_monitor_new_from_netlink(udev_.get(), "udev"),
               udev_monitor_unref),
      dbase_(dal::LocalStorage::GetStorage()), udef_fd_{0} {
  if (!udev_ || !monitor_)
    throw std::runtime_error("Can't connect to udev");
  int res = udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(),
                                                            "usb", NULL);
  if (res < 0)
    throw std::runtime_error("Error modifiing udev filters");
  res = udev_monitor_filter_add_match_subsystem_devtype(monitor_.get(), "block",
                                                        NULL);
  if (res < 0)
    throw std::runtime_error("Error modifiing udev filters");
  res = udev_monitor_enable_receiving(monitor_.get());
  if (res < 0)
    throw std::runtime_error("Error enabling udev monitor");
  udef_fd_ = udev_monitor_get_fd(monitor_.get());
  std::stringstream str_id;
  str_id << std::this_thread::get_id();
  logger_->debug("Udev monitor constructed in thread {}", str_id.str());
}

void UdevMonitor::Run() noexcept {
  // review local mounts
  utils::ReviewMountPoints(logger_);
  uint64_t iteration_counter = 0;
  std::future<bool> fut_review_db;
  while (!StopRequested()) {
    // wait for new device
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(udef_fd_, &fds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000000; // 1sec
    int ret = select(udef_fd_ + 1, &fds, NULL, NULL, &timeout);
    if (ret == -1) {
      logger_->error("select() systemcall returned error");
      logger_->error(strerror(errno));
      continue;
    }
    // timeout
    if (ret == 0) {
      // review table every 10min
      ++iteration_counter;
      if (iteration_counter >= 600) {
        iteration_counter = 0;
        fut_review_db =
            std::async(std::launch::async, utils::ReviewMountPoints, logger_);
      }
      continue;
    }
    // new device found
    if (ret > 0 && FD_ISSET(udef_fd_, &fds)) {
      ProcessDevice();
    }
  }
  logger_->debug("Stop signal recieved");
}

void UdevMonitor::ProcessDevice() noexcept {
  auto device = RecieveDevice();
  if (!device)
    return;
  // there are some rules in db for this device
  bool device_is_known =
      dbase_->permissions
          .Find(dal::Device({device->vid(), device->pid(), device->serial()}))
          .has_value();
  // the device is added
  bool device_was_added = device->action() == Action::kAdd;
  // the device is added + known
  bool known_device_was_added = device_is_known && device_was_added;
  // the device was mounted by this app
  bool device_was_mounted =
      dbase_->mount_points.Find(device->block_name()).has_value();
  // device is removed + was mounted by this app
  bool device_removed_and_was_mounted =
      device_was_mounted && device->action() == Action::kRemove;
  bool fs_is_unsupported = device->filesystem().empty() ||
                           device->filesystem() == "jfs" ||
                           device->filesystem() == "LVM2_member";
  if ((known_device_was_added || device_removed_and_was_mounted) &&
      !fs_is_unsupported) {
    auto begin = std::chrono::high_resolution_clock::now();
    std::string filesystem = device->filesystem();
    utils::MountDevice(std::move(device), logger_);
    std::cout << "Mount device time = " << filesystem << " "
              << utils::since(begin).count() << "[ms]\n";
    return;
  }
  // else - on device change - check the /etc/mtab and compare it with  db
  // maybe local mount table is not valid
  if (device->action() == Action::kChange && device_was_mounted) {
    utils::ReviewMountPoints(logger_);
  }
}

void UdevMonitor::Stop() noexcept { stop_signal_.set_value(); }

bool UdevMonitor::StopRequested() noexcept {
  return !(future_obj_.wait_for(std::chrono::milliseconds(0)) ==
           std::future_status::timeout);
}

std::shared_ptr<UsbUdevDevice> UdevMonitor::RecieveDevice() noexcept {
  std::unique_ptr<udev_device, decltype(&UdevDeviceFree)> device(
      udev_monitor_receive_device(monitor_.get()), UdevDeviceFree);
  if (device) {
    try {
      return std::make_shared<UsbUdevDevice>(std::move(device));
    } catch (const std::exception &ex) {
      // logger_->debug("Constructor of UsbUdevDevice failed");
      // logger_->debug(ex.what());
      return std::shared_ptr<UsbUdevDevice>();
    }
  }
  return std::shared_ptr<UsbUdevDevice>();
}

} // namespace usbmount