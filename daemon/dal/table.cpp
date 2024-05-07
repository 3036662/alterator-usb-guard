#include "table.hpp"
#include "dto.hpp"
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <sys/types.h>
#include <utility>

namespace usbmount::dal {

namespace fs = std::filesystem;

// CRUD Table
Table::Table(const std::string &data_file_path) : file_path_(data_file_path) {
  if (!fs::exists(file_path_)) {
    std::unique_lock<std::shared_mutex> lock(file_mutex_);
    fs::create_directories(fs::path(file_path_).parent_path());
    std::ofstream file(file_path_, std::ios_base::out);
    if (!file.is_open())
      throw std::runtime_error("Can't open " + data_file_path);
    file.close();
    lock.unlock();
  } else {
    ReadRaw();
  }
}

void Table::ReadRaw() {
  if (!fs::exists(file_path_))
    return;
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_)
    lock = std::shared_lock(file_mutex_);
  std::ifstream file(file_path_);
  if (!file.is_open())
    throw std::runtime_error("Can't open " + file_path_);
  raw_json_ = std::string(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
  file.close();
}

void Table::WriteRaw() {
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_)
    lock = std::unique_lock(file_mutex_);
  std::ofstream file(file_path_, std::ios_base::out);
  if (!file.is_open())
    throw std::runtime_error("Can't open " + file_path_);
  file << Serialize();
  file.close();
  if (!transaction_started_)
    lock.unlock();
}

json::value Table::ToJson() const noexcept {
  json::array res;
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_)
    lock = std::shared_lock(data_mutex_);
  for (const auto &entry : data_) {
    json::object js_entry = entry.second->ToJson().as_object();
    js_entry["id"] = entry.first;
    res.emplace_back(std::move(js_entry));
  }
  return res;
}

void Table::CheckIndex(uint64_t index) const {
  std::shared_lock<std::shared_mutex> lock;
  if (!transaction_started_)
    lock = std::shared_lock(data_mutex_);
  if (data_.count(index) == 0)
    throw std::invalid_argument(kWrongArg);
}

void Table::Delete(uint64_t index) {
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_)
    lock = std::unique_lock(data_mutex_);
  data_.erase(index);
  if (!transaction_started_) {
    lock.unlock();
    WriteRaw();
  }
}

uint64_t Table::size() const noexcept { return data_.size(); }

void Table::Clear() {
  std::unique_lock<std::shared_mutex> lock;
  if (!transaction_started_)
    lock = std::unique_lock(data_mutex_);
  data_.clear();
  if (!transaction_started_) {
    lock.unlock();
    WriteRaw();
  }
}

void Table::StartTransaction() noexcept {
  transaction_mutex_.lock();
  transaction_started_ = true;
  transaction_data_lock_ = std::unique_lock(data_mutex_);
  transaction_file_lock_ = std::unique_lock(file_mutex_);
}

bool Table::ProcessTransaction() noexcept {
  try {
    DeepDataClone();
    WriteRaw();
  } catch (const std::exception &ex) {
    std::swap(data_, data_clone_);
    data_clone_.clear();
    transaction_file_lock_.unlock();
    transaction_data_lock_.unlock();
    transaction_mutex_.unlock();
    transaction_started_ = false;
    return false;
  }
  transaction_file_lock_.unlock();
  transaction_data_lock_.unlock();
  transaction_started_ = false;
  transaction_mutex_.unlock();

  return true;
}

void Table::DeepDataClone() {
  data_clone_.clear();
  for (const auto &item : data_) {
    data_clone_.emplace(item.first, item.second->Clone());
  }
}

} // namespace usbmount::dal
