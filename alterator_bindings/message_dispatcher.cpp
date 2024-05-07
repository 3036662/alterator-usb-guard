#include "message_dispatcher.hpp"
#include "common_utils.hpp"
#include "log.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <utility>

using namespace common_utils;

MessageDispatcher::MessageDispatcher(DispatchFunc pfunc) noexcept
    : p_dispatcher_func_(std::move(pfunc)) {}

bool MessageDispatcher::Dispatch(const LispMessage &msg) const noexcept {
  if (p_dispatcher_func_ == nullptr)
    return false;
  return p_dispatcher_func_(msg);
}
