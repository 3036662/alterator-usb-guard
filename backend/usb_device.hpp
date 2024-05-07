#pragma once
#include "serializable_for_lisp.hpp"
#include "types.hpp"
#include <string>

namespace guard {

/**
 * @class UsbDevice
 * @brief Represents a usb device
 */
class UsbDevice : public SerializableForLisp<UsbDevice> {
public:
  /**
   * @brief Basic UsbDevice data set
   */
  struct DeviceData {
    uint num;           /// order number in rules file
    std::string status; /// allow or block
    std::string name;   /// device name
    std::string vid;    /// vendor id
    std::string pid;    /// product id
    std::string port;   /// port
    std::string conn;   /// connection type
    std::string i_type; /// interface type
    std::string sn;     /// serial number
    std::string hash;   /// hash (UsbGuard)
  };

  /**
   * @brief Construct a new Usb Device object
   * @param data A DeviceData obj
   */
  explicit UsbDevice(const DeviceData &data);
  UsbDevice() = delete;
  UsbDevice(const UsbDevice &) noexcept = default;
  UsbDevice(UsbDevice &&) noexcept = default;

  /**
   * @brief Serialize data to vector of pairs
   * html_name : value for displaing in frontend
   * @return Vector of string pairs, suitable for alterator
   * frontend
   */
  vecPairs SerializeForLisp() const;

  inline const std::string &vid() const noexcept { return vid_; };
  inline const std::string &vendor_name() const noexcept {
    return vendor_name_;
  }
  inline void vendor_name(const std::string &str) noexcept {
    vendor_name_ = str;
  }
  inline const std::string &name() const noexcept { return name_; }
  inline const std::string &hash() const noexcept { return hash_; }

private:
  uint number;
  std::string status_;
  std::string name_;
  std::string vid_;
  std::string pid_;
  std::string port_;
  std::string connection_;
  std::string i_type_;
  std::string sn_;
  std::string hash_;
  std::string vendor_name_;
};

/*-------------------------------------------------------------------------*/
/**
 * @class UsbType
 * @brief Represents USB class code
 * @details Information that is used to identify a device’s functionality
 * The information is contained in three bytes
 * with the names Base Class, SubClass, and Protocol. (
 */
class UsbType {
public:
  /// @brief Constructor from string
  /// @param string Fotmatted string 00:00:00
  /// @throws std::invalid_argument  std::out_of_range std::logical_error
  explicit UsbType(const std::string &);
  UsbType(UsbType &&) noexcept = default;
  UsbType &operator=(UsbType &&) noexcept = default;

  inline const unsigned char &base() const noexcept { return base_; };
  inline const std::string &base_str() const noexcept { return base_str_; };
  inline const std::string &sub_str() const noexcept { return sub_str_; };
  inline const std::string &protocol_str() const noexcept {
    return protocol_str_;
  };

private:
  unsigned char base_ = 0;
  unsigned char sub_ = 0;
  unsigned char protocol_ = 0;
  std::string base_str_;
  std::string sub_str_;
  std::string protocol_str_;
};

} // namespace guard