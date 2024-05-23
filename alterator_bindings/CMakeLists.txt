add_library(lisp_bindings OBJECT 
    lisp_message.cpp
    common_utils.cpp
    message_reader.cpp
    message_dispatcher.cpp
)



if(CMAKE_BUILD_TYPE STREQUAL "Debug")

add_custom_target(pre_build DEPENDS $<TARGET_OBJECTS:lisp_bindings>)

include(Format)
Format(pre_build .)

include(ClangTidy)
AddClangTidy(lisp_bindings)

include(CppCheck)
AddCppCheck(lisp_bindings_cppcheck)
add_dependencies(lisp_bindings usbguard_cppcheck)

endif()
