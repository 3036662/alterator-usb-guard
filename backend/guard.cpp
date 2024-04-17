#include "guard.hpp"
#include "config_status.hpp"
#include "guard_rule.hpp"
#include "guard_utils.hpp"
#include "json_changes.hpp"
#include "log.hpp"
#include "usb_device.hpp"
#include "utils.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace guard {

using ::guard::utils::Log;

Guard::Guard() noexcept : ptr_ipc_(nullptr) { ConnectToUsbGuard(); }

bool Guard::HealthStatus() const noexcept {
  return ptr_ipc_ && ptr_ipc_->isConnected();
}

std::vector<UsbDevice> Guard::ListCurrentUsbDevices() noexcept {
  std::vector<UsbDevice> res;
  if (!HealthStatus())
    return res;
  // health is ok -> get all devices
  try {
    std::vector<usbguard::Rule> rules;
    rules = ptr_ipc_->listDevices(kDefaultQuery);
    for (const usbguard::Rule &rule : rules) {
      std::vector<std::string> i_types = guard::utils::FoldUsbInterfacesList(
          rule.attributeWithInterface().toRuleString());
      for (const std::string &i_type : i_types) {
        std::vector<std::string> vid_pid;
        boost::split(vid_pid, rule.getDeviceID().toString(),
                     [](const char symbol) { return symbol == ':'; });
        UsbDevice::DeviceData dev_data{
            rule.getRuleID(),
            usbguard::Rule::targetToString(rule.getTarget()),
            rule.getName(),
            !vid_pid.empty() ? vid_pid[0] : "",
            vid_pid.size() > 1 ? vid_pid[1] : "",
            rule.getViaPort(),
            rule.getWithConnectType(),
            i_type,
            rule.getSerial(),
            rule.getHash()};
        res.emplace_back(std::move(dev_data));
      }
    }
  } catch (const std::exception &ex) {
    Log::Error() << "USBGuard error";
    Log::Error() << ex.what();
    return res;
  }
  // fill all vendor names with one read of file
  std::unordered_set<std::string> vendors_to_search;
  for (const UsbDevice &usb : res) {
    vendors_to_search.insert(usb.vid());
  }
  std::unordered_map<std::string, std::string> vendors_names{
      guard::utils::MapVendorCodesToNames(vendors_to_search)};
  for (UsbDevice &usb : res) {
    if (vendors_names.count(usb.vid()) != 0) {
      usb.vendor_name(vendors_names[usb.vid()]);
    }
  }
  return res;
}

bool Guard::AllowOrBlockDevice(const std::string &device_id, bool allow,
                               bool permanent) noexcept {
  if (device_id.empty() || !HealthStatus())
    return false;
  std::optional<uint32_t> id_numeric = ::utils::StrToUint(device_id);
  if (!id_numeric)
    return false;
  usbguard::Rule::Target policy =
      allow ? usbguard::Rule::Target::Allow : usbguard::Rule::Target::Block;
  try {
    ptr_ipc_->applyDevicePolicy(*id_numeric, policy, permanent);
  } catch (const usbguard::Exception &ex) {
    Log::Error() << "Can't add rule."
                 << "May be rule conflict happened.";
    Log::Error() << ex.what();
    return false;
  }
  return true;
}

ConfigStatus Guard::GetConfigStatus() noexcept {
  ConfigStatus config_status;
  //  TODO if daemon is on active think about creating policy before enabling
  if (!HealthStatus())
    ConnectToUsbGuard();
  config_status.guard_daemon_active(HealthStatus());
  return config_status;
}

void Guard::ConnectToUsbGuard() noexcept {
  try {
    ptr_ipc_ = std::make_unique<usbguard::IPCClient>(true);
  } catch (usbguard::Exception &e) {
    Log::Warning() << "Error connecting to USBGuard daemon.";
    Log::Warning() << e.what();
  }
}

std::optional<std::string>
Guard::ProcessJsonRulesChanges(const std::string &msg,
                               bool apply_changes) noexcept {
  try {
    ConfigStatus config = GetConfigStatus();
    bool initial_active_status = config.guard_daemon_active();
    bool initial_enable_status = config.guard_daemon_enabled();
    Target initial_policy = config.implicit_policy();
    guard::json::JsonChanges js_changes(msg);
    // give a list of active devices if needed
    if (js_changes.ActiveDeviceListNeeded()) {
      ConnectToUsbGuard();
      // if usbguard is not active, activate it with "allow"
      if (!HealthStatus()) {
        Log::Debug() << "[ProcessJson] Starting usbguard with allow policy to "
                        "get list of devices";
        config.ChangeImplicitPolicy(false);
        config.TryToRun(true);
        ConnectToUsbGuard();
        if (!HealthStatus()) {
          throw std::runtime_error("Can't launch usbguard");
        }
      }
      js_changes.active_devices(ListCurrentUsbDevices());
      // after recieving devices, restore initial status
      Log::Debug() << "[ProcessJson] Recovering the initial policy and status";
      config.ChangeImplicitPolicy(initial_policy == Target::block);
      config.ChangeDaemonStatus(initial_active_status, initial_enable_status);
    }
    return js_changes.Process(apply_changes);
  } catch (const std::exception &ex) {
    Log::Error()
        << "Ann error occured while processing changes from web interface";
    Log::Error() << ex.what();
    return std::nullopt;
  }
}

} // namespace guard
