cmake_minimum_required(VERSION 3.14)

include("beforetarget.cmake" OPTIONAL)

file(GLOB BINARY_TEST
    "*.h"
    "*.cpp"
)
message(${BINARY_TEST})
add_executable(binary_test ${BINARY_TEST})
set_target_properties(binary_test PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;bin")
target_link_libraries(binary_test binary_options binary_lib gtest_main)
target_include_directories(binary_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/../src/include/binary ${CMAKE_CURRENT_SOURCE_DIR}/../headers)

set_target_properties(binary_test PROPERTIES OUTPUT_NAME binary_test)

add_custom_command(TARGET binary_test POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:binary_test> ${CMAKE_SOURCE_DIR}/bin/)

