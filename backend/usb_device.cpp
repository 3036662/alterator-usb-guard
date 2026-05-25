#include "usb_device.hpp"

#include <boost/algorithm/string.hpp>
#include <limits>
#include <string>
#include <vector>
namespace guard {

UsbDevice::UsbDevice(const DeviceData &data)
    : number(data.num),
      status_(data.status),
      name_(data.name),
      vid_(data.vid),
      pid_(data.pid),
      port_(data.port),
      connection_(data.conn),
      i_type_(data.i_type),
      sn_(data.sn),
      hash_(data.hash) {}

vecPairs UsbDevice::SerializeForLisp() const {
    vecPairs res;
    res.emplace_back("label_prsnt_usb_number", std::to_string(number));
    res.emplace_back("label_prsnt_usb_port", port_);
    res.emplace_back("label_prsnt_usb_class", i_type_);
    res.emplace_back("label_prsnt_usb_vid", vid_);
    res.emplace_back("label_prsnt_usb_pid", pid_);
    res.emplace_back("label_prsnt_usb_status", status_);
    res.emplace_back("label_prsnt_usb_name", name_);
    // res.emplace_back("label_prsnt_usb_connection", connection);
    res.emplace_back("label_prsnt_usb_serial", sn_);
    res.emplace_back("label_prsnt_usb_hash", hash_);
    res.emplace_back("label_prsnt_usb_vendor", vendor_name_);
    return res;
}

UsbType::UsbType(const std::string &str) {
    std::logic_error ex_common =
        std::logic_error("Can't parse CC::SS::PP " + str);
    std::vector<std::string> splitted;
    boost::split(splitted, str,
                 [](const char symbol) { return symbol == ':'; });
    if (splitted.size() != 3) {
        throw ex_common;
    }

    for (std::string &element : splitted) {
        boost::trim(element);
        if (element.size() > 2) {
            throw ex_common;
        }
    }
    int limit = std::numeric_limits<unsigned char>::max();
    int val = stoi(splitted[0], nullptr, 16);
    if (val >= 0 && val <= limit) {
        base_ = static_cast<unsigned char>(val);
        base_str_ = std::move(splitted[0]);
    } else {
        throw ex_common;
    }
    val = stoi(splitted[1], nullptr, 16);
    if (val >= 0 && val <= limit) {
        sub_ = static_cast<unsigned char>(val);
        sub_str_ = std::move(splitted[1]);
    } else {
        throw ex_common;
    }
    val = stoi(splitted[2], nullptr, 16);
    if (val >= 0 && val <= limit) {
        protocol_ = static_cast<unsigned char>(val);
        protocol_str_ = std::move(splitted[2]);
    } else {
        throw ex_common;
    }
}

}  // namespace guard