function(AddCppCheck target)
find_program(CPPCHECK_EXECUTABLE cppcheck REQUIRED)
if(CPPCHECK_EXECUTABLE)
    add_custom_target(${target}
        COMMAND ${CPPCHECK_EXECUTABLE} --enable=all  --check-level=exhaustive --inconclusive --force --inline-suppr --template=gcc --std=c++17 --suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.cppcheck  ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running cppcheck on all files in folder ${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()
    message(WARNING "cppcheck not found. Please install cppcheck to enable static code analysis.")
endif()

endfunction()