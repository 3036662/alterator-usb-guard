
add_executable(test
               run.cpp 
               test.cpp 
               ${CMAKE_SOURCE_DIR}/alterator_bindings/common_utils.cpp
               ../backend/usb_device.cpp
               ../backend/config_status.cpp
               ../backend/guard.cpp
               ../backend/guard_rule.cpp
               ../backend/json_rule.cpp
               ../backend/guard_utils.cpp
               ../backend/csv_rule.cpp
               ../backend/json_changes.cpp
               ../backend/guard_audit.cpp
               )

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/alterator_bindings)               
target_include_directories(test PRIVATE ../backend)
target_include_directories(test PRIVATE .)
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/common)

include_directories(test PUBLIC ${USBGUARD_INCLUDE_DIRS})
target_include_directories(test PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/cppcodec/cppcodec)
target_include_directories(test PUBLIC  ${CMAKE_SOURCE_DIR}/thirdparty/rapidcsv/src)

target_link_libraries(test PRIVATE boost_json)
target_link_libraries(test PRIVATE log_reader)
target_link_libraries(test PRIVATE systemd_dbus)
target_link_libraries(test PRIVATE PkgConfig::USBGUARD)
target_link_libraries(test PRIVATE SDBusCpp::sdbus-c++)
