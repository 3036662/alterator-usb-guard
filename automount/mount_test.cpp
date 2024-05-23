
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libudev.h>
#include <memory>
#include <string>
// #include <libmount/libmount.h>
#include <mntent.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

bool Remove(const std::string &dev, std::ofstream &log);

int main(int argc, const char *argv[]) {
  std::ofstream log("/home/oleg/mount_log.txt", std::ios_base::app);
  if (argc < 3)
    return 1;
  std::string dev = argv[1];
  if (dev.empty()) {
    return 1;
  }
  std::string action = argv[2];
  if (action.empty()) {
    return 1;
  }

  //****************************************************************

  dev = "/dev/" + dev;
  log << "-------------------------------------\n\n";
  log << dev << "\n";
  log << action << "\n";

  // action add
  if (action == "add") {
    //  find a device in udev
    std::unique_ptr<udev, decltype(&udev_unref)> udev{udev_new(), udev_unref};
    if (!udev) {
      log << "Failed to create udev object\n";
      log.close();
      return 1;
    }
    std::unique_ptr<udev_enumerate, decltype(&udev_enumerate_unref)> enumerate{
        udev_enumerate_new(udev.get()), udev_enumerate_unref};
    udev_enumerate_add_match_subsystem(enumerate.get(), "block");
    udev_enumerate_scan_devices(enumerate.get());
    udev_list_entry *entry = udev_enumerate_get_list_entry(enumerate.get());

    /**************************************************************************/
    // find a device
    udev_device *ptr_device = nullptr;
    while (entry != NULL) {
      const char *p_path = udev_list_entry_get_name(entry);
      std::unique_ptr<udev_device, decltype(&udev_device_unref)> device(
          udev_device_new_from_syspath(udev.get(), p_path), udev_device_unref);
      if (!device) {
        log << "Failed to find device " << p_path << "\n";
        continue;
      }
      const char *devnode = udev_device_get_devnode(device.get());
      if (devnode == NULL)
        continue;
      // a device found
      if (dev == std::string(devnode)) {
        ptr_device = device.release();
      }
      entry = udev_list_entry_get_next(entry);
    }

    /*****************************************************************************/
    // device found -get info

    std::unique_ptr<udev_device, decltype(&udev_device_unref)> device(
        ptr_device, udev_device_unref);

    const char *p_label =
        udev_device_get_property_value(device.get(), "ID_FS_LABEL");
    std::string label;
    if (p_label != NULL)
      label = p_label;
    std::string uid;
    const char *p_uid =
        udev_device_get_property_value(device.get(), "ID_PART_ENTRY_UUID");
    if (p_uid != NULL)
      uid = p_uid;
    std::string filesystem;
    const char *p_fs =
        udev_device_get_property_value(device.get(), "ID_FS_TYPE");
    if (p_fs != NULL)
      filesystem = p_fs;

    std::string mount_point = "/media/oleg/";
    if (!label.empty()) {
      mount_point += label;
    } else if (!uid.empty()) {
      mount_point += uid;
    } else {
      mount_point = "usb";
    }

    std::string dev_type;
    const char *p_devtipe =
        udev_device_get_property_value(device.get(), "DEVTYPE");
    if (p_devtipe != NULL) {
      dev_type = p_devtipe;
    }
    // ID_PART_ENTRY_NUMBER
    int partitions = 0;
    const char *p_partition_count =
        udev_device_get_property_value(device.get(), "ID_PART_ENTRY_NUMBER");
    if (p_partition_count != NULL) {
      partitions = std::stoi(p_partition_count);
    }
    log << dev_type << " " << partitions;
    uint counter = 0;
    while (std::filesystem::exists(mount_point)) {
      ++counter;
      mount_point += std::to_string(counter);
    }

    /*************************************************************/
    //  device indo is ready. act

    if (filesystem.empty()) {
      log << "filesystem is empty\n";
      log.close();
      return 0;
    }
    if (dev_type == "disk") {
      log << dev << " is a disk, skip\n";
      log.close();
      return 0;
    }

    const uint owner = 1000;
    const uint group = 1000;

    if (!std::filesystem::create_directory(mount_point)) {
      log << "Can't create a directory " << mount_point << "\n";
    }

    if (chmod(mount_point.c_str(), S_IRWXU | S_IRWXG) != 0) {
      log << "Can't set permissions to mount_point\n";
    }
    chown(mount_point.c_str(), owner, group);

    log << "Mount point for " << dev << " = " << mount_point << "\n";
    log << "Filesystem is " << filesystem << "\n";
    log << "Source is " << dev << "\n";

    while (!std::filesystem::exists(dev)) {
      log << dev << "not exists\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    log << dev << " exists\n";

    // std::string opts = "uid=1000,gid=1001,umask=007";
    std::string opts;
    ulong flags = 0;
    if (filesystem == "iso9660") {
      flags = MS_NOSUID | MS_RDONLY | MS_RELATIME;
    }

    int res = mount(dev.c_str(), mount_point.c_str(), filesystem.c_str(), flags,
                    opts.c_str());
    if (res != 0) {
      log << "Error mounting " << dev << "\n";
      log << std::strerror(errno) << "\n";
    } else {
      log << "OK\n";
    }
    if (std::filesystem::is_empty(mount_point)) {
      std::filesystem::remove(mount_point);
      log << "Remove " << mount_point << "\n";
    }

  } else if (action == "remove") {
    Remove(dev, log);
  }
  log.close();
  return 0;
}

bool Remove(const std::string &dev, std::ofstream &log) {
  // find mount point
  FILE *p_file = setmntent("/etc/mtab", "r");
  if (p_file == NULL) {
    log << "Error opening /etc/mtab";
    return false;
  }
  mntent *entry;
  std::string mount_point;
  while ((entry = getmntent(p_file)) != NULL) {
    if (std::string(entry->mnt_fsname) == dev) {
      mount_point = entry->mnt_dir;
      break;
    }
  }
  endmntent(p_file);

  if (mount_point.empty()) {
    log << dev << " is not mounted\n";
    log.close();
    return true;
  }

  log << "mount point for unmount is " << mount_point << "\n";
  //
  int res = umount2(mount_point.c_str(), 0);
  if (res != 0) {
    log << "Error unmounting " << dev << "\n";
    log << std::strerror(errno) << "\n";
    return false;
  }
  if (std::filesystem::is_empty(mount_point)) {
    std::filesystem::remove(mount_point);
    log << "Remove " << mount_point << "\n";
  }
  return true;
}
