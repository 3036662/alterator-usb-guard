add_executable(altusbd 
    main.cpp
)

add_library(daemon_libs OBJECT
     utils.cpp
     custom_mount.cpp
     usb_udev_device.cpp
     daemon.cpp
     udev_monitor.cpp
     dbus_methods.cpp
     #udisks_dbus.cpp 
)

add_subdirectory(dal)
target_link_libraries(altusbd PRIVATE DAL)
target_link_libraries(altusbd PRIVATE boost_json)
target_link_libraries(altusbd PRIVATE daemon_libs)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   add_subdirectory(test)
   include(Format)
   Format(altusbd .)
   include(ClangTidy)
   AddClangTidy(altusbd)
   # add_dependencies(altusbd run_cppcheck)
   AddCppCheck(daemon_cppcheck)
   add_dependencies(altusbd daemon_cppcheck)
endif()

include_directories(altusbd PUBLIC ${SYSTEMD_INCLUDE_DIRS})
target_link_libraries(altusbd PRIVATE PkgConfig::SYSTEMD)
target_link_libraries(altusbd PRIVATE SDBusCpp::sdbus-c++)

target_link_libraries(altusbd PRIVATE PkgConfig::UDEV)
target_include_directories(altusbd PUBLIC ${UDEV_INCLUDE_DIRS})

# acl 
pkg_check_modules(ACL REQUIRED IMPORTED_TARGET libacl)
target_link_libraries(altusbd PRIVATE PkgConfig::ACL)
target_include_directories(altusbd PUBLIC ${ACL_INCLUDE_DIRS})

# logger
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
target_link_libraries(altusbd PRIVATE fmt)


install(TARGETS altusbd
    RUNTIME DESTINATION "/usr/bin"
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/daemon/service_files/altusbd.service
    DESTINATION "/lib/systemd/system/"
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/daemon/service_files/ru.alterator.usbd.conf
    DESTINATION "/usr/share/dbus-1/system.d/"
)

add_subdirectory(polkit)

