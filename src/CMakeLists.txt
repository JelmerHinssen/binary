cmake_minimum_required(VERSION 3.14)

include("beforetarget.cmake" OPTIONAL)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.cpp " ")
file(GLOB BINARY_SRC
    "*.h"
    "*.cpp"
)
list(APPEND BINARY_SRC ${CMAKE_CURRENT_BINARY_DIR}/null.cpp)
list(REMOVE_ITEM BINARY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/memory.cpp)
list(REMOVE_ITEM BINARY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/file.cpp)
add_library(binary_options INTERFACE)

if(MSVC)
    target_compile_options(binary_options INTERFACE /W4)
    target_compile_definitions(binary_options INTERFACE DEBUG_MODE=$<CONFIG:Debug>)
else()
    target_compile_options(binary_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_options(binary_options INTERFACE -g)
        target_compile_definitions(binary_options INTERFACE DEBUG_MODE=1)
        else()
        target_compile_options(binary_options INTERFACE -O2)
        target_compile_definitions(binary_options INTERFACE DEBUG_MODE=0)
    endif()
endif()

add_library(binary_lib STATIC ${BINARY_SRC})
add_library(binary_lib_alloc STATIC "memory.cpp" "file.cpp")
add_library(binary_lib_alloc_stack STATIC "memory.cpp" "file.cpp")
add_library(binary_lib_stack STATIC "memory.cpp" "file.cpp")
add_executable(binary_run "main.cpp" "file.cpp")

target_include_directories(binary_options INTERFACE include)
target_include_directories(binary_options INTERFACE include/shilm)

target_compile_definitions(binary_lib_alloc PUBLIC SHILM_ENABLE_ALLOCATOR)
target_compile_definitions(binary_lib_alloc_stack PUBLIC SHILM_ENABLE_ALLOCATOR SHILM_ENABLE_STACKTRACE)
target_compile_definitions(binary_lib_stack PUBLIC SHILM_ENABLE_STACKTRACE)

target_include_directories(binary_run PRIVATE include/shilm)
target_link_libraries(binary_lib binary_options ${all_libraries})
target_link_libraries(binary_lib_alloc binary_options ${all_libraries})
target_link_libraries(binary_lib_stack binary_options ${all_libraries} Dbghelp.lib)
target_link_libraries(binary_lib_alloc_stack binary_options ${all_libraries} Dbghelp.lib)
target_link_libraries(binary_run binary_lib binary_lib_stack binary_options)

set_target_properties(binary_run PROPERTIES OUTPUT_NAME binary)
set_target_properties(binary_lib PROPERTIES OUTPUT_NAME binary)
set_target_properties(binary_lib_alloc PROPERTIES OUTPUT_NAME binary_alloc)
set_target_properties(binary_lib_alloc_stack PROPERTIES OUTPUT_NAME binary_alloc_stack)
set_target_properties(binary_lib_stack PROPERTIES OUTPUT_NAME binary_stack)

add_custom_command(TARGET binary_run POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:binary_run> ${CMAKE_SOURCE_DIR}/bin/)
add_custom_command(TARGET binary_lib POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:binary_lib> ${CMAKE_SOURCE_DIR}/bin/)
add_custom_command(TARGET binary_lib_alloc POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:binary_lib_alloc> ${CMAKE_SOURCE_DIR}/bin/)
add_custom_command(TARGET binary_lib_alloc_stack POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:binary_lib_alloc_stack> ${CMAKE_SOURCE_DIR}/bin/)
add_custom_command(TARGET binary_lib_stack POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:binary_lib_stack> ${CMAKE_SOURCE_DIR}/bin/)
