#include "daemon.hpp"
#include "dbus_methods.hpp"
#include "udev_monitor.hpp"
// #include "udisks_dbus.hpp"
#include "utils.hpp"
#include <csignal>
#include <memory>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/Message.h>
#include <sdbus-c++/StandardInterfaces.h>
#include <sdbus-c++/sdbus-c++.h>
#include <thread>

namespace usbmount {

Daemon::Daemon()
    : is_running_(true), reload_(false),
      logger_(utils::InitLogFile("/var/log/alt-usb-automount/log.txt")),
      udev_(std::make_shared<UdevMonitor>(logger_)),
      dbus_methods_(udev_, logger_) {}

bool Daemon::IsRunning() noexcept {
  if (reload_) {
    reload_ = false;
    Daemon::Reload();
  }
  return is_running_;
}

void Daemon::SignalHandler(int signal) noexcept {
  switch (signal) {
  case SIGINT:
  case SIGTERM: {
    Daemon::instance().is_running_ = false;
    break;
  }
  case SIGHUP: {
    Daemon::instance().reload_ = true;
    break;
  }
  default:
    break;
  }
}

void Daemon::Run() {
  dbus_methods_.Run(); // non blocking - Async
  std::thread thread_monitor(&UdevMonitor::Run, udev_.get());
  sigset_t signal_set;
  int signal_number;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set, SIGTERM);
  sigaddset(&signal_set, SIGHUP);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);
  while (IsRunning()) {
    sigwait(&signal_set, &signal_number);
    SignalHandler(signal_number);
  }
  udev_->Stop();
  thread_monitor.join();
  logger_->info("stopped the Daemon loop");
}

} // namespace usbmount