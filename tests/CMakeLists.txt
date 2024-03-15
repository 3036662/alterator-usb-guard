
add_executable(test
               run.cpp 
               test.cpp 
               ../backend/utils.cpp
               ../backend/usb_device.cpp
               ../backend/config_status.cpp
               ../backend/guard.cpp
               ../backend/systemd_dbus.cpp
               ../backend/guard_rule.cpp
               ../backend/systemd_dbus.cpp
               ../backend/json_rule.cpp
               ../backend/guard_utils.cpp
               ../backend/csv_rule.cpp
               ../backend/json_changes.cpp
               )
target_include_directories(test PRIVATE ../backend)
target_include_directories(test PRIVATE .)
include_directories(test PUBLIC ${USBGUARD_INCLUDE_DIRS})
target_include_directories(test PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/cppcodec/cppcodec)
target_include_directories(test PUBLIC  ${CMAKE_SOURCE_DIR}/thirdparty/rapidcsv/src)

target_link_libraries(test PRIVATE boost_json)
target_link_libraries(test PRIVATE PkgConfig::USBGUARD)
target_link_libraries(test PRIVATE SDBusCpp::sdbus-c++)
