#pragma once
#include "dto.hpp"
#include <boost/json/value.hpp>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace usbmount::dal {

// CRUD table
class Table : public Dto {
public:
  Table(const std::string &data_file_path);
  json::value ToJson() const noexcept override;

  void Clear();
  uint64_t size() const noexcept;

  virtual void Create(const Dto &) = 0;
  virtual const Dto &Read(uint64_t) const = 0;
  virtual void Update(uint64_t, const Dto &) = 0;
  void Delete(uint64_t);
  virtual ~Table() = default;

  /**
   * @brief Transactions can be used for modifing method - CREATE,UPDATE,DELETE
   */
  void StartTransaction() noexcept;
  bool ProcessTransaction() noexcept;

protected:
  void CheckIndex(uint64_t index) const;
  void WriteRaw();

  std::mutex transaction_mutex_;
  bool transaction_started_ = false;
  std::string raw_json_;
  static constexpr const char *kWrongArg = "no data with such index";
  std::map<uint64_t, std::shared_ptr<Dto>> data_;
  mutable std::shared_mutex data_mutex_;
  std::shared_mutex file_mutex_;

private:
  void ReadRaw();
  virtual void DataFromRawJson() = 0;
  void DeepDataClone();

  const std::string file_path_; // path to data file

  std::map<uint64_t, std::shared_ptr<Dto>> data_clone_;
  std::unique_lock<std::shared_mutex> transaction_data_lock_;
  std::unique_lock<std::shared_mutex> transaction_file_lock_;
};

} // namespace usbmount::dal