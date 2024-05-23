#pragma once

#include "types.hpp"

/// @brief CRTP base class for serializable objects
/// @tparam Impl Implementation class
template <typename Impl> class SerializableForLisp {
public:
  vecPairs SerializeForLisp() const {
    return static_cast<const Impl *>(this)->SerializeForLisp();
  }
};

template <> class SerializableForLisp<vecPairs> {
  vecPairs vec;
public:
  explicit SerializableForLisp(vecPairs &&vec_) : vec{std::move(vec_)} {};
  vecPairs SerializeForLisp() const { return vec; }
};

/// @brief Interface for polimorphic classes 
class ISerializableForLisp{
public:
  virtual vecPairs SerializeForLisp() const noexcept=0;
};