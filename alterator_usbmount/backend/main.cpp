#include "dispatcher_impl.hpp"
#include "lisp_message.hpp"
#include "message_reader.hpp"
#include "usb_mount.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  using namespace ::alterator::usbmount;
  UsbMount mount;
  DispatcherImpl dispatcher(mount);
  auto dispatcher_func = [&dispatcher](const LispMessage &msg) {
    return dispatcher.Dispatch(msg);
  };
  MessageReader reader(dispatcher_func);
  reader.Loop();
  return 0;
}