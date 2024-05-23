add_library(DAL OBJECT
    local_storage.cpp
    dto.cpp
    table.cpp
    device_permissions.cpp
    mount_points.cpp
)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(pre_build DEPENDS $<TARGET_OBJECTS:DAL>)
    include(Format)
    Format(pre_build .)
    include(ClangTidy)
    AddClangTidy(DAL)
    include(CppCheck)
    AddCppCheck(dal_cppcheck)
    add_dependencies(DAL dal_cppcheck)
endif()

IF (DEFINED UNIT_TEST)
    message("Building tests...")
    add_subdirectory(test)
ENDIF (DEFINED UNIT_TEST)