#include "test.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  std::cout << "Tests..." << std::endl;
  Test test;
  test.Run1();
  //  test2 passes only if usbguard is active and service is enabled
  // test.Run2();
  //  test3 passes only if usbguard is installed
  test.Run3();
  //  test4 passes only if usbguard is installed
  //  needs root
  test.Run4();
  /// test5 passes only if usbguard is installed
  // needs root
  test.Run5();
  // test6 passes only if usbguard is installed and config is default
  // needs root
  test.Run6();
  // test 7 test string  	with-interface { 0e:01:00 0e:02:00 0e:02:00
  // 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 0e:02:00 } folding t0
  // ["0e:*:*"] vector
  test.Run7();
  // test usb vendor names lookup
  test.Run8();

  // test a raw string rule splitting
  test.Run9();
  // Test a rule string parsing to a GuardRule object and backwards.
  test.Run10();

  // // Test a rules file parsing
  test.Run11();

  // // Test parsing of json array of uints
  //test.Run12(); DEPRECATED

  // // Test systemd restart and stop
  test.Run13();

  // // Test parsing json rule object
  test.Run14();

  // // Test guard ParseJsonRulesChanges
  test.Run15();

  // // Test ConfigStatus::ChangeImplicitPolicy
  test.Run16();

  // // Test Guard:: ProcessJsonAllowConnected
  test.Run17();

  // test Guard Audit reading
  test.Run18();

  return 0;
}
