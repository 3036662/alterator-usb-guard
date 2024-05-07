add_executable(alterator_usb_automount
#    mount_test.cpp
    main.cpp
    utils.cpp
    usb_udev_device.cpp
    custom_mount.cpp
)
target_link_libraries(alterator_usb_automount PRIVATE PkgConfig::UDEV)


find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(PkgConfig)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)
pkg_check_modules(ACL REQUIRED IMPORTED_TARGET libacl)
target_link_libraries(alterator_usb_automount PRIVATE ${SYSTEMD_LIBRARIES})
target_link_libraries(alterator_usb_automount PRIVATE fmt)
target_link_libraries(alterator_usb_automount PRIVATE PkgConfig::ACL)
target_include_directories(alterator_usb_automount PUBLIC ${ACL_INCLUDE_DIRS})

#target_link_libraries(alterator_usb_automount PRIVATE spdlog)










install(TARGETS alterator_usb_automount
    RUNTIME DESTINATION "/usr/bin/"
)

install(FILES 
    ${CMAKE_SOURCE_DIR}/automount/service_files/alterator-usb-mount@.service
    DESTINATION "/lib/systemd/system/"
)
install(FILES 
    ${CMAKE_SOURCE_DIR}/automount/service_files/alterator-usb-automount.conf
    DESTINATION "/lib/tmpfiles.d/"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(alterator_usb_automount .)
    include(ClangTidy)
    AddClangTidy(alterator_usb_automount)
    AddCppCheck(automount_cppcheck)
    add_dependencies(alterator_usb_automount automount_cppcheck)
endif()


include_directories(alterator_usb_automount PUBLIC ${SYSTEMD_INCLUDE_DIRS})

target_link_libraries(alterator_usb_automount PRIVATE PkgConfig::UDEV)
target_include_directories(alterator_usb_automount PUBLIC ${UDEV_INCLUDE_DIRS})
