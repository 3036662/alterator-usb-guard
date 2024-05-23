add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/backend)


install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/USBMount.desktop
    DESTINATION "/usr/share/alterator/applications/"
)

install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/ajax.scm 
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/index.html 
    DESTINATION "/usr/share/alterator/ui/usbmount"
)

install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/alt_usb_mount.css
    DESTINATION "/usr/share/alterator/design/styles"
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/usbmount_ui/alt_usb_mount.js
    DESTINATION "/usr/share/alterator/design/scripts"
)


set(BUILD_MO_COMMAND "msgfmt --output-file=${CMAKE_BINARY_DIR}/alterator-usbmount.mo ${CMAKE_SOURCE_DIR}/alterator_usbmount/usbmount_ui/alterator-usbmount.po)")
message("Command ${BUILD_MO_COMMAND}")
install(CODE "execute_process(COMMAND ${BUILD_MO_COMMAND}")
install(FILES
    ${CMAKE_BINARY_DIR}/alterator-usbmount.mo
    DESTINATION "/usr/share/locale/ru/LC_MESSAGES/"
)