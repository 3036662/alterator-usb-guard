add_library(boost_json OBJECT boost_json.cpp)

add_library(systemd_dbus OBJECT systemd_dbus.cpp )

add_library(log_reader OBJECT log_reader.cpp )

target_include_directories(systemd_dbus PRIVATE ${CMAKE_SOURCE_DIR}/alterator_bindings)

target_include_directories(log_reader PRIVATE ${CMAKE_SOURCE_DIR}/alterator_bindings)