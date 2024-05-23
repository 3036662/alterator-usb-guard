add_compile_definitions(UNIT_TEST=1)
find_package(Catch2  REQUIRED)
add_executable(test_daemon test.cpp)
target_link_libraries(test_daemon PRIVATE Catch2::Catch2)
target_include_directories(test_daemon PUBLIC "${CATCH2_INCLUDE_DIR}")
target_include_directories(test_daemon PUBLIC ${CMAKE_SOURCE_DIR}/daemon/ )
target_include_directories(test_daemon PUBLIC ${CMAKE_SOURCE_DIR}/alterator_bindings/ )
target_link_libraries(test_daemon PRIVATE daemon_libs)
target_link_libraries(test_daemon PRIVATE boost_json)
target_link_libraries(test_daemon PRIVATE DAL)
target_include_directories(test_daemon PUBLIC ${CMAKE_SOURCE_DIR}/alterator_bindings/ )


include(CTest)
include(Catch)
catch_discover_tests(test_daemon)

pkg_check_modules(ACL REQUIRED IMPORTED_TARGET libacl)
target_link_libraries(test_daemon PRIVATE PkgConfig::ACL)
target_include_directories(test_daemon PUBLIC ${ACL_INCLUDE_DIRS})
target_link_libraries(test_daemon PRIVATE PkgConfig::UDEV)
target_link_libraries(test_daemon PRIVATE SDBusCpp::sdbus-c++)

# logger
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
target_link_libraries(test_daemon PRIVATE fmt)