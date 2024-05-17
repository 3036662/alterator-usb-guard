#include "dbus_methods.hpp"
#include "dal/dto.hpp"
#include "dal/local_storage.hpp"
#include "udev_monitor.hpp"
#include "usb_udev_device.hpp"
#include "utils.hpp"
#include <algorithm>
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <cstdint>
#include <exception>
#include <iterator>
#include <sdbus-c++/Message.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <vector>

namespace usbmount {

namespace json = boost::json;

DbusMethods::DbusMethods(std::shared_ptr<UdevMonitor> udev_monitor,
                         std::shared_ptr<spdlog::logger> logger)
    : connection_(sdbus::createSystemBusConnection(service_name)),
      dbus_object_ptr(sdbus::createObject(*connection_, object_path)),
      logger_(std::move(logger)), dbase_(dal::LocalStorage::GetStorage()),
      udev_monitor_(std::move(udev_monitor)) {
  // Health()
  dbus_object_ptr->registerMethod(interface_name, "health", "", "s", Health);
  // ListDevices()
  dbus_object_ptr->registerMethod(
      interface_name, "ListDevices", "", "s",
      [this](const sdbus::MethodCall &call) { ListActiveDevices(call); });
  // ListRules
  dbus_object_ptr->registerMethod(
      interface_name, "ListRules", "", "s",
      [this](const sdbus::MethodCall &call) { ListActiveRules(call); });
  // Get users and groups
  dbus_object_ptr->registerMethod(
      interface_name, "GetUsersAndGroups", "", "s",
      [this](const sdbus::MethodCall &call) { GetSystemUsersAndGroups(call); });
  // CanAnotherUserUnmount("/dev/sd..")
  dbus_object_ptr->registerMethod(interface_name, "CanAnotherUserUnmount", "s",
                                  "s", [this](sdbus::MethodCall call) {
                                    CanAnotherUserUnmount(std::move(call));
                                  });
  // CanUserMount("/dev/sd..")
  dbus_object_ptr->registerMethod(
      interface_name, "CanUserMount", "s", "s",
      [this](sdbus::MethodCall call) { CanUserMount(std::move(call)); });
  // SaveRules (json string)
  dbus_object_ptr->registerMethod(
      interface_name, "SaveRules", "s", "s",
      [this](sdbus::MethodCall call) { SaveRules(std::move(call)); });
  dbus_object_ptr->finishRegistration();
}

void DbusMethods::Run() { connection_->enterEventLoopAsync(); }

void DbusMethods::Health(const sdbus::MethodCall &call) {
  auto reply = call.createReply();
  reply << "OK";
  reply.send();
}

void DbusMethods::CanAnotherUserUnmount(sdbus::MethodCall call) {
  std::string dev;
  call >> dev;
  logger_->info("Polkit request for device (CanUserUnMount)" + dev);
  auto reply = call.createReply();
  // if this device was mounted by this program - reply YES
  try {
    auto index = dbase_->mount_points.Find(dev);
    if (index) {
      reply << "YES";
      logger_->info("Daemon response to polkit = YES");
    } else {
      reply << "UNKNOWN_MOUNTPOINT";
      logger_->info("Daemon response to polkit = UNKNOWN_MOUNTPOINT");
    }
  } catch (const std::exception &ex) {
    logger_->error("Dbus::CanUserUnmount Can't query the dbase for device + {}",
                   dev);
  }
  logger_->flush();
  reply.send();
}

void DbusMethods::CanUserMount(sdbus::MethodCall call) {
  std::string dev;
  call >> dev;
  /*
  Read local db define is this user is allowed to mount this drive
  (if drive is in db - reply NO - automount will mount it )
  */
  logger_->info("Polkit request for device (CanUserMount)" + dev);
  sdbus::MethodReply reply = call.createReply();
  try {
    UsbUdevDevice device({dev, "add"});
    dal::Device dto_device({device.vid(), device.pid(), device.serial()});
    if (dbase_->permissions.Find(dto_device)) {
      logger_->info("daemon reply to polkit = NO");
      reply << "NO";
    } else {
      logger_->info("daemon reply to polkit = YES");
      reply << "YES";
    }
    logger_->flush();
    reply.send();
    return;
  } catch (const std::exception &ex) {
    logger_->error("[Dbus::CanUserMount]Can't construct udev device from " +
                   dev);
  }
  // just in case  -must be unreacheable
  reply << "YES";
  reply.send();
}

void DbusMethods::ListActiveDevices(const sdbus::MethodCall &call) {
  logger_->debug("[DBUS][ListActiveDevices]");
  auto devices = udev_monitor_->GetConnectedDevices();
  json::array response_array;
  for (const auto &dev : devices) {
    json::object obj;
    obj["device"] = dev.block_name();
    obj["vid"] = dev.vid();
    obj["pid"] = dev.pid();
    obj["serial"] = dev.serial();
    obj["fs"] = dev.filesystem();
    // mount_point
    auto index_mount = dbase_->mount_points.Find(dev.block_name());
    if (index_mount)
      obj["mount"] = dbase_->mount_points.Read(*index_mount).mount_point();
    else
      obj["mount"] = "";
    // permissions
    auto perm_index = dbase_->permissions.Find(
        dal::Device({dev.vid(), dev.pid(), dev.serial()}));
    perm_index.has_value() ? obj["status"] = "owned" : obj["status"] = "free";
    response_array.emplace_back(std::move(obj));
  }
  sdbus::MethodReply reply = call.createReply();
  // logger_->debug("[DBUS] Response = {}", json::serialize(response_array));
  reply << json::serialize(response_array);
  reply.send();
}

void DbusMethods::ListActiveRules(const sdbus::MethodCall &call) {
  logger_->debug("[DBUS][ListActiveRules]");
  json::array response_array;
  auto rules = dbase_->permissions.getAll();
  for (auto &rule : rules) {
    json::object obj;
    obj["id"] = std::to_string(rule.first);
    obj["perm"] = rule.second->ToJson();
    response_array.emplace_back(std::move(obj));
  }
  sdbus::MethodReply reply = call.createReply();
  reply << json::serialize(response_array);
  reply.send();
}

void DbusMethods::GetSystemUsersAndGroups(const sdbus::MethodCall &call) {
  logger_->debug("[DBUS][GetUsersAndGroups]");
  json::object res;
  const auto id_limits = utils::GetSystemUidMinMax(logger_);
  if (id_limits.has_value()) {
    auto users = utils::GetHumanUsers(id_limits.value(), logger_);
    if (!std::empty(users)) {
      res["users"] = json::array();
      for (const auto &usr : users)
        res["users"].as_array().emplace_back(usr.ToJson());
    }
    auto groups = utils::GetHumanGroups(id_limits.value(), logger_);
    if (!groups.empty()) {
      res["groups"] = json::array();
      for (const auto &grp : groups)
        res["groups"].as_array().emplace_back(grp.ToJson());
    }
  }
  sdbus::MethodReply reply = call.createReply();
  reply << json::serialize(res);
  reply.send();
}

void DbusMethods::SaveRules(sdbus::MethodCall call) {
  logger_->debug("[DBUS][SaveRules]");
  json::object res;
  // TODO
  std::string form_data;
  call >> form_data;
  try {
    // parse data to json object
    const json::value val = json::parse(form_data);
    const json::object &json_data = val.as_object();
    dbase_->permissions.StartTransaction();
    // delete rules
    if (json_data.contains("deleted") && json_data.at("deleted").is_array()) {
      json::array arr_deleted = json_data.at("deleted").as_array();
      logger_->debug("[DBUS][SaveRules] Deleted");
      for (const auto &element : arr_deleted) {
        uint64_t id_to_delete = utils::StrToUint(element.as_string().c_str());
        dbase_->permissions.Delete(id_to_delete);
      }
    }
    // update rules
    if (json_data.contains("updated") && json_data.at("updated").is_array()) {
      json::array arr_updated = json_data.at("updated").as_array();
      logger_->debug("[DBUS][SaveRules] Updated");
      UpdateRules(arr_updated);
    }
    // create rules
    if (json_data.contains("created") && json_data.at("created").is_array()) {
      json::array arr_created = json_data.at("created").as_array();
      logger_->debug("[DBUS][SaveRules] Created");
      CreateRules(arr_created);
    }
    dbase_->permissions.ProcessTransaction();
    logger_->debug("[DBUS][SaveRules] Finished transacion");
  } catch (const std::exception &ex) {
    logger_->error(ex.what());
  }
  logger_->debug("[DBUS][SaveRules]{}", form_data);
  logger_->flush();

  res["STATUS"] = "OK";
  sdbus::MethodReply reply = call.createReply();
  reply << json::serialize(res);
  reply.send();
}

void DbusMethods::CreateRules(const boost::json::array &arr_created) {
  auto id_limits = utils::GetSystemUidMinMax(logger_);
  std::vector<dal::User> system_users;
  std::vector<dal::Group> system_groups;
  if (id_limits) {
    system_users = utils::GetHumanUsers(id_limits.value(), logger_);
    system_groups = utils::GetHumanGroups(id_limits.value(), logger_);
  }

  for (const auto &element : arr_created) {
    const json::object &obj = element.as_object();
    std::string vid = obj.at("vid").as_string().c_str();
    std::string pid = obj.at("pid").as_string().c_str();
    std::string serial = obj.at("serial").as_string().c_str();
    std::string user = obj.at("user").as_string().c_str();
    std::string group = obj.at("group").as_string().c_str();
    auto it_system_user = std::find_if(
        system_users.cbegin(), system_users.cend(),
        [&user](const dal::User &usr) { return usr.name() == user; });
    bool valid_user = it_system_user != system_users.cend();
    auto it_system_group = std::find_if(
        system_groups.cbegin(), system_groups.cend(),
        [&group](const dal::Group &grp) { return grp.name() == group; });
    bool valid_group = it_system_group != system_groups.cend();
    if (!utils::ValidVid(vid) || !utils::ValidVid(pid) || serial.empty() ||
        user.empty() || group.empty() || !valid_group || !valid_user) {
      throw std::invalid_argument("invalid arguments for device permissions");
    }
    std::vector<dal::User> new_users{*it_system_user};
    std::vector<dal::Group> new_groups{*it_system_group};
    dal::PermissionEntry new_entry(dal::Device({vid, pid, serial}),
                                   std::move(new_users), std::move(new_groups));
    dbase_->permissions.Create(new_entry);
  }
}

void DbusMethods::UpdateRules(const boost::json::array &arr_updated) {
  auto id_limits = utils::GetSystemUidMinMax(logger_);
  std::vector<dal::User> system_users;
  std::vector<dal::Group> system_groups;
  if (id_limits) {
    system_users = utils::GetHumanUsers(id_limits.value(), logger_);
    system_groups = utils::GetHumanGroups(id_limits.value(), logger_);
  }
  for (const auto &element : arr_updated) {
    const json::object &obj = element.as_object();
    uint64_t id_to_update = utils::StrToUint(obj.at("id").as_string().c_str());
    // const std::string vid=
    std::string vid = obj.at("vid").as_string().c_str();
    std::string pid = obj.at("pid").as_string().c_str();
    std::string serial = obj.at("serial").as_string().c_str();
    std::string user = obj.at("user").as_string().c_str();
    std::string group = obj.at("group").as_string().c_str();
    // get original  perm copy
    json::object original =
        dbase_->permissions.Read(id_to_update).ToJson().as_object();
    if (!vid.empty())
      original.at("device").as_object().at("vid") = vid;
    if (!pid.empty())
      original.at("device").as_object().at("pid") = pid;
    if (!serial.empty())
      original.at("device").as_object().at("serial") = serial;
    if (!user.empty() && !system_users.empty()) {
      auto it_system_user = std::find_if(
          system_users.cbegin(), system_users.cend(),
          [&user](const dal::User &usr) { return usr.name() == user; });
      if (it_system_user != system_users.cend()) {
        original.at("users").as_array()[0].as_object().at("uid") =
            it_system_user->uid();
        original.at("users").as_array()[0].as_object().at("name") =
            it_system_user->name();
      }
    }
    if (!group.empty() && !system_groups.empty()) {
      auto it_system_group = std::find_if(
          system_groups.cbegin(), system_groups.cend(),
          [&group](const dal::Group &grp) { return grp.name() == group; });
      if (it_system_group != system_groups.cend()) {
        original.at("groups").as_array()[0].as_object().at("gid") =
            it_system_group->gid();
        original.at("groups").as_array()[0].as_object().at("name") =
            it_system_group->name();
      }
    }
    // update data
    dbase_->permissions.Update(id_to_update, dal::PermissionEntry(original));
  }
}

} // namespace usbmount