#pragma once
#include "dto.hpp"
#include "table.hpp"
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace usbmount::dal {

class DevicePermissions : public Table {
public:
  /**
   * @brief Construct a new DevicePermissions object
   * @param path Path to a file to store data
   * @throws runtime_error
   */
  explicit DevicePermissions(const std::string &);

  // CRUD

  /**
   * @brief  Create a new entry for a permisson in the local storage
   * @throws bad_cast , runtime_error
   */
  void Create(const Dto &) override;

  /**
   * @brief Get entry by index
   * @return const MountEntry&
   * @throws std::invalid_argument (wrong index), out_of_range by std::map
   */
  const PermissionEntry &Read(uint64_t) const override;

  /**
   * @brief Update entry by index
   * @throws bad_cast, invalid_argument (index)
   */
  void Update(uint64_t, const Dto &) override;

  /**
   * @brief Find by Device object
   * @param dev dal::Device object
   * @return std::optional<uint64_t> index or empty if nothing was found
   */
  std::optional<uint64_t> Find(const Device &dev) const noexcept;

  std::map<uint64_t, std::shared_ptr<const PermissionEntry>>
  getAll() const noexcept;

private:
  /**
   * @brief Read raw_json_ and fill the fields with data
   * @throws runtime_error, system_error (json  parser)
   */
  void DataFromRawJson() override;

  // no cloning
  inline std::shared_ptr<Dto> Clone() const noexcept override {
    return nullptr;
  };
};

} // namespace usbmount::dal
