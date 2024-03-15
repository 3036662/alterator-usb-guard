#pragma once

#include "config_status.hpp"
#include "guard.hpp"
#include "guard_rule.hpp"
#include "usb_device.hpp"
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include <optional>
#include <string>
#include <vector>

namespace guard::json {

namespace json = boost::json;

class JsonChanges {
public:
  explicit JsonChanges(const std::string &msg);

  inline bool ActiveDeviceListNeeded() const noexcept {
    return preset_mode_ == "put_connected_to_white_list" ||
           preset_mode_ == "put_connected_to_white_list_plus_HID";
  }

  inline void active_devices(std::vector<UsbDevice> &&devices) {
    active_devices_ = std::move(devices);
  }

  std::string Process(bool apply);

private:
  void ExtractDaemonTargetState();
  void ExtractTargetPolicy();
  void ExtractPresetMode();

  /**
   * @brief fills obj_result_,rules_to_delete_, rules_deleted_,rules_to_add_
   * @throws std::logic_error, std::runtime_error
   */
  void ProcessManualMode();

  /**
   * @brief puts new rules to rules_to_add_,
   * @details fills obj_result_ with "rules_OK" and "rules_BAD" ids
   * @throws std::logic_error
   */
  void ProcessJsonAppended();

  /**
   * @brief Delete rules by indexes rules_to_delete_
   * @details puts delete rules indexes to rules_deleted_
   * pushes the rest of old rules to new_rules
   * @throws std::logic_error if error pasing old rules
   */
  void DeleteRules();

  /**
   * @brief put all old rules to rules_to_delete_ (for presets)
   * @throws std::logic_error if error pasing old rules
   */
  void DeleteAllOldRules();

  /**
   * @brief Add active devices to rules_to_add_
   * @throws  std::logic_error if no device list is set
   */
  void ProcessAllowConnected();

  /**
   * @brief Add allow HID devices to rules_to_add_
   * @throws std::logic_error
   */
  void AddAllowHid();

  /**
   * @brief Add allow 08 and 06 interfaces to rules_to_add_
   * @throws std::logic_error
   */
  void AddBlockUsbStorages();

  /**
   * @brief Add androide devicese to rules_to_add_ with "block"
   *
   */
  void AddBlockAndroid();

  boost::json::object AddAppendByPresetToResponse() const;

  // default init
  std::string preset_mode_;
  std::vector<GuardRule> new_rules_;
  std::vector<uint> rules_to_delete_;
  std::vector<uint> rules_deleted_;
  std::vector<GuardRule> rules_to_add_;
  std::vector<GuardRule> added_by_preset_; // just for displaing to user
  ConfigStatus config_;
  boost::json::object obj_result_;
  std::optional<std::vector<UsbDevice>> active_devices_;
  json::value json_value_;

  json::object *p_jobj_;
  bool daemon_activate_;
  Target target_policy_;
  bool rules_changed_by_policy_;
};

} // namespace guard::json
