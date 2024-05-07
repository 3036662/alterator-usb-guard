add_executable(altusbmount_askdbus
    main.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(Format)
    Format(altusbmount_askdbus .)
    include(ClangTidy)
    AddClangTidy(altusbmount_askdbus)
    AddCppCheck(polkit_cppcheck)
    add_dependencies(altusbmount_askdbus polkit_cppcheck)
endif()

target_link_libraries(altusbmount_askdbus PRIVATE SDBusCpp::sdbus-c++)


install(TARGETS altusbmount_askdbus
    RUNTIME DESTINATION "/usr/libexec"
)


install(FILES 
    ${CMAKE_SOURCE_DIR}/daemon/polkit/alt-usb-mount.rules
    DESTINATION "/etc/polkit-1/rules.d"
)