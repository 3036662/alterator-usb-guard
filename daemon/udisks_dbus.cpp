#include "udisks_dbus.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/Types.h>
#include <thread>
#include <vector>

namespace usbmount {

UdisksDbus::UdisksDbus(std::unique_ptr<sdbus::IConnection> &conn)
    : dbus_(conn),
      proxy_udisks_(sdbus::createProxy(*dbus_, "org.freedesktop.UDisks2",
                                       "/org/freedesktop/UDisks2/Manager")) {}

bool UdisksDbus::ProcessDevice(const UsbUdevDevice &dev) {
  std::cerr << dev.block_name() << "\n";
  auto method = proxy_udisks_->createMethodCall(
      "org.freedesktop.UDisks2.Manager", "ResolveDevice");
  {
    std::map<std::string, sdbus::Variant> param;
    param.emplace("path", dev.block_name());
    method << param << std::map<std::string, sdbus::Variant>();
  }
  std::vector<sdbus::ObjectPath> obj_path;
  for (uint attempt = 0; attempt < 10; ++attempt) {
    sdbus::MethodReply reply = proxy_udisks_->callMethod(method);
    reply >> obj_path;
    if (!obj_path.empty()) {
      std::cerr << obj_path[0] << "\n";
      break;
    }
    std::cerr << "Sleep 300ms ..."
              << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  // mount on behalf of another user

  auto dev_proxy =
      sdbus::createProxy(*dbus_, "org.freedesktop.UDisks2", obj_path[0]);

  auto introspect_method = dev_proxy->createMethodCall(
      "org.freedesktop.DBus.Introspectable", "Introspect");
  auto introspection_res = dev_proxy->callMethod(introspect_method);
  std::string xml_introspection;
  introspection_res >> xml_introspection;
  if (boost::contains(xml_introspection,
                      "org.freedesktop.UDisks2.Filesystem")) {
    std::cerr << "Filesystem interface exists"
              << "\n";

    std::vector<std::vector<uint8_t>> mountpoints =
        dev_proxy->getProperty("MountPoints")
            .onInterface("org.freedesktop.UDisks2.Filesystem");
    while (!mountpoints.empty()) {
      std::cerr << "Already mounted "
                << "\n";
      auto unmount_method = dev_proxy->createMethodCall(
          "org.freedesktop.UDisks2.Filesystem", "Unmount");
      std::map<std::string, sdbus::Variant> param;
      param.emplace("force", sdbus::Variant(true));
      unmount_method << param;
      auto reply = dev_proxy->callMethod(unmount_method);
      std::cerr << "Unmount...OK "
                << "\n";
      mountpoints = dev_proxy->getProperty("MountPoints")
                        .onInterface("org.freedesktop.UDisks2.Filesystem");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    auto mount_method = dev_proxy->createMethodCall(
        "org.freedesktop.UDisks2.Filesystem", "Mount");

    std::map<std::string, sdbus::Variant> param1;
    param1.emplace("as-user", sdbus::Variant("test"));
    mount_method << param1;
    auto reply = dev_proxy->callMethod(mount_method);
    std::string mount_path;
    reply >> mount_path;
    std::cerr << "Mounted to " << mount_path << " for user test"
              << "\n";
  }

  // ****************** writes to fstab
  // auto dev_proxy =
  //     sdbus::createProxy(*dbus_, "org.freedesktop.UDisks2", obj_path[0]);
  // auto dev_method =
  // dev_proxy->createMethodCall("org.freedesktop.UDisks2.Block",
  //                                               "AddConfigurationItem");
  // // sdbus::struct<std::string, std::map<std::string, sdbus::Variant>>
  // param1; std::map<std::string, sdbus::Variant> map1;

  // //  map1.emplace("fsname", sdbus::Variant(dev.block_name.c_str()));
  // std::string dir;
  // std::vector<uint8_t> vec_dir;
  // for (char symbol : dir)
  //   vec_dir.push_back(static_cast<uint8_t>(symbol));
  // vec_dir.push_back(0);
  // std::string type = "auto";
  // std::vector<uint8_t> vec_type;
  // for (char symbol : type)
  //   vec_type.push_back(static_cast<uint8_t>(symbol));
  // vec_type.push_back(0);
  // std::string opts = "uid=500,gid=500,umask=007";
  // std::vector<uint8_t> vec_opts;
  // for (char symbol : opts)
  //   vec_opts.push_back(static_cast<uint8_t>(symbol));
  // vec_opts.push_back(0);

  // map1.emplace("dir", sdbus::Variant(vec_dir));
  // map1.emplace("type", sdbus::Variant(vec_type));
  // map1.emplace("opts", sdbus::Variant(vec_opts));
  // map1.emplace("freq", sdbus::Variant(0));
  // map1.emplace("passno", sdbus::Variant(0));

  // std::cerr << "opts size="
  //           << map1.at("opts").get<std::vector<uint8_t>>().size() << "\n";
  // sdbus::Struct<std::string, std::map<std::string, sdbus::Variant>> sturct2{
  //     "fstab", map1};

  // // param1.emplace("fstab", map1);
  // dev_method << sturct2;
  // std::map<std::string, sdbus::Variant> vardict;
  // vardict.emplace("auth.no_user_interaction", true);
  // dev_method << vardict;
  // auto reply = dev_proxy->callMethod(dev_method);

  return obj_path.empty();
}

} // namespace usbmount