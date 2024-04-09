#include "device_permissions.hpp"
#include <boost/json/object.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
#include <cstddef>
#include <fstream>
#include <future>
#include <sstream>
#define CATCH_CONFIG_MAIN
#include "dto.hpp"
#include "local_storage.hpp"
#include <boost/json.hpp>
#include <catch2/catch.hpp>
#include <filesystem>
#include <string>
#include <thread>

using namespace usbmount::dal;
namespace fs = std::filesystem;

// clang-format off
TEST_CASE("Test DTO objects"){
  SECTION("Device"){
    {
      Device dev;
      REQUIRE(dev.Serialize()=="{\"vid\":\"\",\"pid\":\"\",\"serial\":\"\"}");      
    }
    {
      Device dev({"aa10","bb10","saodik"});
      REQUIRE(dev.Serialize()=="{\"vid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");      
    }
    {
      REQUIRE_THROWS(
        Device({"xx","bb10","sdfsdfsdf"})
      );      
    }
    {
      boost::json::value val=boost::json::parse("{\"vIIIid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");
      REQUIRE_THROWS(
         Device(val.as_object())
      );      
    }
    {
      boost::json::value val=boost::json::parse("{\"vid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");
      Device dev(val.as_object());
      REQUIRE(dev.Serialize()=="{\"vid\":\"aa10\",\"pid\":\"bb10\",\"serial\":\"saodik\"}");      
    }
  }

  SECTION("User"){
    {
      User user;
      REQUIRE(user.Serialize()=="{\"uid\":0,\"name\":\"\"}");
    }
    {
      REQUIRE_THROWS(
        User(0,"")
      );  
    }
    {
      User user(0,"rooot");
      REQUIRE(user.Serialize()=="{\"uid\":0,\"name\":\"rooot\"}");
    }
     {
       boost::json::value val=boost::json::parse("{\"uid\":0,\"name\":\"rooot\"}");
       User user(val.as_object());
       REQUIRE(user.Serialize()=="{\"uid\":0,\"name\":\"rooot\"}");      
     }
  }


  SECTION("Group"){
    {
      Group group;
      REQUIRE(group.Serialize()=="{\"gid\":0,\"name\":\"\"}");
    }
    {
      REQUIRE_THROWS(
        User(0,"")
      );  
    }
    {
      Group group(0,"rooot");
      REQUIRE(group.Serialize()=="{\"gid\":0,\"name\":\"rooot\"}");
    }
     {
       boost::json::value val=boost::json::parse("{\"gid\":0,\"name\":\"rooot\"}");
       Group Group(val.as_object());
       REQUIRE(Group.Serialize()=="{\"gid\":0,\"name\":\"rooot\"}");      
     }
  }

  SECTION("MountEntry"){
    REQUIRE(MountEntry().Serialize() =="{\"dev_name\":\"\",\"mount_point\":\"\",\"fs_type\":\"\"}");
    MountEntry entry({"/dev/sda","/mount","ntfs"});
    boost::json::object obj=entry.ToJson().as_object();
    REQUIRE(MountEntry(obj).Serialize()=="{\"dev_name\":\"/dev/sda\",\"mount_point\":\"/mount\",\"fs_type\":\"ntfs\"}");
    REQUIRE_THROWS(MountEntry({"","",""}));
  }

  SECTION("PermissionEntry"){
     std::string js_string=  "{\"device\":{\"vid\":\"00\",\"pid\":\"0000\",\"serial\":\"234958098\"},"
                            "\"users\":[{\"uid\":0,\"name\":\"root\"}],"
                            "\"groups\":[{\"gid\":500,\"name\":\"groupName\"}]}";
     REQUIRE(PermissionEntry(DeviceParams{"00","0000","234958098"},
                            {{0,"root"}},{{500,"groupName"}}).Serialize()==js_string);  
     REQUIRE(PermissionEntry(json::parse(js_string).as_object()).Serialize()==js_string);                                            
     REQUIRE_THROWS(PermissionEntry(DeviceParams{"0z0","0000","234958098"},
                            {{0,"root"}},{{500,"groupName"}}));
     REQUIRE_THROWS(PermissionEntry(DeviceParams{"00","00z00","234958098"},
                            {{0,"root"}},{{500,"groupName"}}));
     REQUIRE_THROWS(PermissionEntry(DeviceParams{"00","0000","234958098"},
                            {{0,""}},{{500,"groupName"}}));     
     REQUIRE_THROWS(PermissionEntry(DeviceParams{"00","0000","234958098"},
                            {{0,"root"}},{{500,""}}));
     REQUIRE_THROWS(PermissionEntry(DeviceParams{"00","0000","234958098"},
                            {},{}));                                                      
  }
}


TEST_CASE("Test local storage") {
  SECTION("Test storage singleton from different threads") {
    std::future<std::shared_ptr<LocalStorage>> fut1 =
        std::async(std::launch::async, LocalStorage::GetStorage);
    std::future<std::shared_ptr<LocalStorage>> fut2 =
        std::async(std::launch::async, LocalStorage::GetStorage);
    auto storage0 = LocalStorage::GetStorage();
    fut1.wait();
    fut2.wait();
    REQUIRE(fut1.valid());
    REQUIRE(fut2.valid());
    auto storage1 = fut1.get();
    auto storage2 = fut2.get();
    REQUIRE(storage1 == storage2);
    REQUIRE(storage1 == storage0);
    REQUIRE(storage2 == storage0);
  }

  const std::string path_perm = "/var/lib/alt-usb-mount/permissions.json";
  const std::string path_mounts = "/var/lib/alt-usb-mount/mount_points.json";

  SECTION("Check json files existance") {
    LocalStorage::GetStorage();
    REQUIRE(fs::exists(path_mounts));
    REQUIRE(fs::exists(path_perm));
  }

  SECTION("DevicePermissions") {
    auto dbase=LocalStorage::GetStorage ();
    REQUIRE(fs::exists(path_mounts));
    REQUIRE(fs::exists(path_perm));
    dbase->permissions.Clear();
    REQUIRE(dbase->permissions.size()==0);
    REQUIRE(dbase->permissions.Serialize()=="[]");

     dbase->permissions.Create(
       PermissionEntry(DeviceParams{"00","0000","234958098"},
                             {{0,"root"}},{{500,"groupName"}})
     );
    dbase->permissions.Create(
      PermissionEntry(DeviceParams{"00d","00da","0000"},
                            {{1,"test"}},{{501,"groupName2"}})
    );  
    REQUIRE(dbase->permissions.size()==2);
    std::string str_js = "[{\"device\":{\"vid\":\"00\",\"pid\":\"0000\",\"serial\":\"234958098\"},\"users\":[{\"uid\":0,\"name\":\"root\"}],\"groups\":[{\"gid\":500,\"name\":\"groupName\"}],\"id\":0},{\"device\":{\"vid\":\"00d\",\"pid\":\"00da\",\"serial\":\"0000\"},\"users\":[{\"uid\":1,\"name\":\"test\"}],\"groups\":[{\"gid\":501,\"name\":\"groupName2\"}],\"id\":1}]";
     REQUIRE(dbase->permissions.Serialize()==str_js);
    {
    std::ifstream file(path_perm);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(str_js==string_stream.str());
    }
    {
    LocalStorage::GetStorage()->permissions.Delete(1);
    std::ifstream file(path_perm);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(string_stream.str()=="[{\"device\":{\"vid\":\"00\",\"pid\":\"0000\",\"serial\":\"234958098\"},\"users\":[{\"uid\":0,\"name\":\"root\"}],\"groups\":[{\"gid\":500,\"name\":\"groupName\"}],\"id\":0}]");
    }
    {
     PermissionEntry perms(json::parse("{\"device\":{\"vid\":\"011\",\"pid\":\"1111\",\"serial\":\"aaa234958098\"},\"users\":[{\"uid\":0,\"name\":\"root\"}],\"groups\":[{\"gid\":500,\"name\":\"groupName\"}],\"id\":0}").as_object());
    LocalStorage::GetStorage()->permissions.Update(0,perms);
    std::ifstream file(path_perm);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(string_stream.str()=="[{\"device\":{\"vid\":\"011\",\"pid\":\"1111\",\"serial\":\"aaa234958098\"},\"users\":[{\"uid\":0,\"name\":\"root\"}],\"groups\":[{\"gid\":500,\"name\":\"groupName\"}],\"id\":0}]");
    REQUIRE(perms.Serialize()==LocalStorage::GetStorage()->permissions.Read(0).Serialize());
     }

    {
    LocalStorage::GetStorage()->permissions.Clear();
    std::ifstream file(path_perm);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(string_stream.str()=="[]");
    }
  }

  SECTION("MountPoints") {
    auto dbase=LocalStorage::GetStorage ();
    REQUIRE(fs::exists(path_mounts));
    REQUIRE(fs::exists(path_perm));
    dbase->mount_points.Clear();
    REQUIRE(dbase->mount_points.size()==0);
    REQUIRE(dbase->mount_points.Serialize()=="[]");

    dbase->mount_points.Create(
      MountEntry({"/dev/sda1","/mount1","ntfs"})
    );

    dbase->mount_points.Create(
      MountEntry({"/dev/sda3","/mount1","vfat"})
    );
      
    REQUIRE(dbase->mount_points.size()==2);
    std::string str_js = "[{\"dev_name\":\"/dev/sda1\",\"mount_point\":\"/mount1\",\"fs_type\":\"ntfs\",\"id\":0},{\"dev_name\":\"/dev/sda3\",\"mount_point\":\"/mount1\",\"fs_type\":\"vfat\",\"id\":1}]";
    REQUIRE(dbase->mount_points.Serialize()==str_js);
    {
    std::ifstream file(path_mounts);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(str_js==string_stream.str());
    }
    {
    LocalStorage::GetStorage()->mount_points.Delete(1);
    std::ifstream file(path_mounts);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(string_stream.str()=="[{\"dev_name\":\"/dev/sda1\",\"mount_point\":\"/mount1\",\"fs_type\":\"ntfs\",\"id\":0}]");
    }
    {
    MountEntry mount_entry(json::parse("{\"dev_name\":\"/dev/sda12\",\"mount_point\":\"/mount1\",\"fs_type\":\"ntfs\",\"id\":0}").as_object());
    LocalStorage::GetStorage()->mount_points.Update(0,mount_entry);
    std::ifstream file(path_mounts);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(string_stream.str()=="[{\"dev_name\":\"/dev/sda12\",\"mount_point\":\"/mount1\",\"fs_type\":\"ntfs\",\"id\":0}]");
    REQUIRE(mount_entry.Serialize()==LocalStorage::GetStorage()->mount_points.Read(0).Serialize());
    }

    {
    LocalStorage::GetStorage()->mount_points.Clear();
    std::ifstream file(path_mounts);
    std::stringstream string_stream;
    string_stream<< file.rdbuf();
    file.close();
    REQUIRE(string_stream.str()=="[]");
    }
  }
}

TEST_CASE("Write from threads"){
  SECTION ("CreatePermissions"){
    LocalStorage::GetStorage()->permissions.Clear();
    auto fut1=std::async(std::launch::async,[](){
      auto dbase= LocalStorage::GetStorage(); 

      for (size_t i=0;i<20;++i){
      dbase->permissions.Create(
          PermissionEntry(DeviceParams{"00","0000","234958098"},
                              {{0,"root"}},{{500,"groupName"}})
      );
      }
    });
    auto fut2=std::async(std::launch::async,[](){
      auto dbase= LocalStorage::GetStorage(); 
      for (size_t i=0;i<20;++i){
      dbase->permissions.Create(
          PermissionEntry(DeviceParams{"00","0000","234958098"},
                              {{0,"root"}},{{500,"groupName"}})
      );
      }
    });
    fut1.wait();
    fut2.wait();
    REQUIRE(LocalStorage::GetStorage()->permissions.size()==40);
  }

  SECTION ("CreateMountpoints"){
    LocalStorage::GetStorage()->mount_points.Clear();
    auto fut1=std::async(std::launch::async,[](){
      auto dbase= LocalStorage::GetStorage(); 

      for (size_t i=0;i<20;++i){
      dbase->mount_points.Create(
         MountEntry({"/dev/sda0","/mount1","vfat"})
      );
      }
    });
    auto fut2=std::async(std::launch::async,[](){
      auto dbase= LocalStorage::GetStorage(); 
      for (size_t i=0;i<20;++i){
      dbase->mount_points.Create(
          MountEntry({"/dev/sda3","/mount1","vfat"})
      );
      }
    });
    fut1.wait();
    fut2.wait();
    REQUIRE(LocalStorage::GetStorage()->mount_points.size()==40);
    LocalStorage::GetStorage()->permissions.Clear();
    LocalStorage::GetStorage()->mount_points.Clear();
  }
}


