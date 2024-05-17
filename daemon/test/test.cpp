#define CATCH_CONFIG_MAIN
#include "utils.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Test utils") {
  using namespace usbmount::utils;
  auto logger = InitLogFile("/var/log/alt-usb-automount/log.txt");
  SECTION("Test /etc/login.defs parsing") {
    auto res = GetSystemUidMinMax(logger);
    REQUIRE(res.has_value());
    REQUIRE(res->uid_min > 0);
    REQUIRE(res->uid_max > 0);
    REQUIRE(res->gid_min > 0);
    REQUIRE(res->gid_max > 0);
  }

  SECTION("string to uint64") {
    REQUIRE(StrToUint("12984798") == 12984798);
    REQUIRE(StrToUint("012984798") == 12984798);
    REQUIRE(StrToUint("000000012984798") == 12984798);
    REQUIRE_THROWS(StrToUint(""));
    REQUIRE_THROWS(StrToUint("12984798d"));
    REQUIRE_THROWS(StrToUint("-12984798"));
    REQUIRE_THROWS(StrToUint("12-984798"));
  }

  SECTION("read possible shells") {
    auto shells = GetPossibleShells(logger);
    REQUIRE(!shells.empty());
    for (const auto &shell : shells) {
      REQUIRE(!shell.empty());
      REQUIRE(shell[0] == '/');
    }
  }

  SECTION("Read human users from the system") {
    auto id_limits = GetSystemUidMinMax(logger);
    REQUIRE(id_limits.has_value());
    auto users = GetHumanUsers(id_limits.value(), logger);
    REQUIRE(!users.empty());
    for (const auto &user : users) {
      REQUIRE(!user.name().empty());
      REQUIRE(user.uid() >= id_limits->uid_min);
      REQUIRE(user.uid() <= id_limits->uid_max);
    }
  }

  SECTION("Read human groups for  the system") {
    auto id_limits = GetSystemUidMinMax(logger);
    REQUIRE(id_limits.has_value());
    auto groups = GetHumanGroups(id_limits.value(), logger);
    REQUIRE(!groups.empty());
    for (const auto &group : groups) {
      // std::cout << group.name() << "\n";
      REQUIRE(!group.name().empty());
      REQUIRE(group.gid() >= id_limits->gid_min);
      REQUIRE(group.gid() <= id_limits->gid_max);
    }
  }

  SECTION("Vid validator") {
    using namespace usbmount::utils;
    REQUIRE(ValidVid("0000"));
    REQUIRE(ValidVid("ffff"));
    REQUIRE(ValidVid("a0b2"));
    REQUIRE(!ValidVid("a0x0"));
    REQUIRE(!ValidVid("-000"));
  }

  SECTION("Mount string sanitize") {
    using namespace usbmount::utils;
    REQUIRE(SanitizeMount("sdlfkjs\"d/l\'kf\\j") == "sdlfkjs_d_l_kf_j");
    REQUIRE(SanitizeMount("sdlfkjssdfsdfa24332&&dfs") ==
            "sdlfkjssdfsdfa24332&&dfs");
  }
}