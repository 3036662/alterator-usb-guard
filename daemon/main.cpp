
#include "daemon.hpp"
#include <chrono>
#include <exception>
#include <iostream>
#include <sdbus-c++/Error.h>
#include <systemd/sd-daemon.h>
#include <thread>

// grab a bus name as last step of initialization.

// make your daemon bus-activatable by supplying
// a D-Bus service activation configuration file

int main() {
  try {
    usbmount::Daemon &daemon = usbmount::Daemon::instance();
    daemon.Run();
  } catch (const std::exception &ex) {
    std::cerr << SD_ERR << "Can't start the daemon ";
    std::cerr << SD_ERR << ex.what();
  }
  std::cerr << SD_DEBUG << "The daemon process ended gracefully.";
  return 0;
}