
add_library(boost_json OBJECT boost_json.cpp)

add_executable(usbguard main.cpp
    lisp_message.hpp lisp_message.cpp
    message_reader.hpp message_reader.cpp
    message_dispatcher.cpp message_dispatcher.hpp
    utils.cpp utils.hpp
    usb_device.cpp usb_device.hpp
    guard.cpp guard.hpp
    config_status.cpp config_status.hpp
    systemd_dbus.cpp systemd_dbus.hpp
    guard_rule.cpp guard_rule.cpp
    json_rule.cpp
    guard_utils.cpp
    csv_rule.cpp
)



if(CMAKE_BUILD_TYPE STREQUAL "Debug")

include(Format)
Format(usbguard .)

include(ClangTidy)
AddClangTidy(usbguard)

include(CppCheck)
AddCppCheck(${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(usbguard run_cppcheck)

endif()



include_directories(usbguard PUBLIC ${USBGUARD_INCLUDE_DIRS})
include_directories(usbguard PUBLIC $CMAKE_SOURCE_DIR/tests)
target_include_directories(usbguard PUBLIC  ${CMAKE_SOURCE_DIR}/thirdparty/cppcodec/cppcodec)
target_include_directories(usbguard PUBLIC  ${CMAKE_SOURCE_DIR}/thirdparty/rapidcsv/src)


target_link_libraries(usbguard PRIVATE boost_json)
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

install(CODE "execute_process(COMMAND service alteratord restart)")
install(CODE "execute_process(COMMAND service ahttpd restart)")
install(CODE "execute_process(COMMAND systemctl restart alteratord )")
install(CODE "execute_process(COMMAND systemctl restart ahttpd )")