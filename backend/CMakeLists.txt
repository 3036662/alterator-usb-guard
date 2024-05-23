
add_executable(usbguard 
    main.cpp
    dispatcher_impl.cpp
    usb_device.cpp 
    guard.cpp 
    config_status.cpp 
    guard_rule.cpp 
    json_rule.cpp
    guard_utils.cpp
    csv_rule.cpp
    json_changes.cpp
    guard_audit.cpp
)





if(CMAKE_BUILD_TYPE STREQUAL "Debug")

include(Format)
Format(usbguard .)

include(ClangTidy)
AddClangTidy(usbguard)

include(CppCheck)
AddCppCheck(usbguard_cppcheck)
add_dependencies(usbguard usbguard_cppcheck)

endif()



include_directories(usbguard PUBLIC ${USBGUARD_INCLUDE_DIRS})
target_include_directories(usbguard PUBLIC  ${CMAKE_SOURCE_DIR}/thirdparty/cppcodec/cppcodec)
target_include_directories(usbguard PUBLIC  ${CMAKE_SOURCE_DIR}/thirdparty/rapidcsv/src)
target_include_directories(usbguard PUBLIC  ${CMAKE_SOURCE_DIR}/alterator_bindings)
target_include_directories(usbguard PRIVATE ${CMAKE_SOURCE_DIR}/common)


target_link_libraries(usbguard PRIVATE boost_json)
target_link_libraries(usbguard PRIVATE systemd_dbus)
target_link_libraries(usbguard PRIVATE lisp_bindings)
target_link_libraries(usbguard PRIVATE log_reader)
target_link_libraries(usbguard PRIVATE PkgConfig::USBGUARD)
target_link_libraries(usbguard PRIVATE SDBusCpp::sdbus-c++)


install(TARGETS usbguard
    RUNTIME DESTINATION "/usr/lib/alterator/backend3"
)
install(FILES 
        ${CMAKE_SOURCE_DIR}/alterator_ui/ajax.scm 
        ${CMAKE_SOURCE_DIR}/alterator_ui/index.html 
        DESTINATION "/usr/share/alterator/ui/usbguard"
)
install(FILES 
    ${CMAKE_SOURCE_DIR}/alterator_ui/alt_usb_guard.css
    DESTINATION "/usr/share/alterator/design/styles"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/alterator_ui/alt-usb-guard.js
    DESTINATION "/usr/share/alterator/design/scripts"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/alterator_ui/alt-usb-guard.js
    DESTINATION "/usr/share/alterator/design/scripts"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/android_vidpid_parser/android_vidpid.json
    DESTINATION "/etc/usbguard/"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/alterator_ui/USBGuard.desktop
    DESTINATION "/usr/share/alterator/applications/"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/alterator_ui/usbguard.html
    DESTINATION "/usr/share/alterator/help/ru_RU/"
)




set(BUILD_MO_COMMAND "msgfmt --output-file=${CMAKE_BINARY_DIR}/alterator-usbguard.mo ${CMAKE_SOURCE_DIR}/alterator_ui/alterator-usbguard.po)")
message("Command ${BUILD_MO_COMMAND}")
install(CODE "execute_process(COMMAND ${BUILD_MO_COMMAND}")
install(FILES
    ${CMAKE_BINARY_DIR}/alterator-usbguard.mo
    DESTINATION "/usr/share/locale/ru/LC_MESSAGES/"
)



install(CODE "execute_process(COMMAND service alteratord restart)")
install(CODE "execute_process(COMMAND service ahttpd restart)")
install(CODE "execute_process(COMMAND systemctl restart alteratord )")
install(CODE "execute_process(COMMAND systemctl restart ahttpd )")