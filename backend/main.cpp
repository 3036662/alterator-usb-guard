#include "dispatcher_impl.hpp"
#include "guard.hpp"
#include "lisp_message.hpp"
#include "message_dispatcher.hpp"
#include "message_reader.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  guard::Guard guard;
  guard::DispatcherImpl impl(guard);
  auto dispatcher_func = [&impl](const LispMessage &msg) {
    return impl.Dispatch(msg);
  };
  MessageReader reader(dispatcher_func);
  reader.Loop();
  return 0;
}
