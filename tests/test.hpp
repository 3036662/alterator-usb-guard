#pragma once

class Test {
public:

  /// @brief test Udev rules inspection
  void Run1();

  /**
   * @brief  test daemon CheckConfig
   * @details to pass this test must have root privileges
   */
  void Run2();

  /**
   * @brief  test daemon CheckConfig find config file for daemon
   * @details to pass this test usbguard package must be installed
   */
  void Run3();

  /**
   * @brief  Parse config file to find path to rules.file
   * @details to pass this test usbguard package must be installed
   * @warning Needs root privileges
   */
  void Run4();

  /**
   * @brief  Parse config file to find allowed users
   * @details to pass this test usbguard package must be installed with defualt
   * config
   * @warning Needs root privileges
   */
  void Run5();

  /**
   * @brief  Parse config file to find allowed groups
   * @details Default group "wheel" is expected
   * @warning Needs root privileges
   */
  void Run6();

  /**
   * @brief  Test folding of list of a interface to vector with masks
   */
  void Run7();

  /**
   * @brief  Test reading vendors from /usr/share/misc/usb.ids
   */
  void Run8();

  /**
   * @brief  test a raw string rule splitting
   *
   */
  void Run9();

  /**
   * @brief Test a rule string parsing to a GuardRule object and backwards.
   *
   */
  void Run10();

  /**
   * @brief Test a rule file parsing
   *
   */
  void Run11();


  /**
   * @brief Test systemd restart and stop
   *
   */
  void Run13();

  /**
   * @brief Test parsing json rule object
   *
   */
  void Run14();

  /**
   * @brief ParseJsonRulesChanges
   *
   */
  void Run15();

  /**
   * @brief test ConfigStatus:: ChangeImplicitPolicy
   *
   */
  void Run16();

  /**
   * @brief  Test Guard:: ProcessJsonAllowConnected
   *
   */
  void Run17();

  /**
   * @brief UdbGuard audit reading from file
   * 
   */
  
  void Run18();
};