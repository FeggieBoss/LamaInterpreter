cmake_minimum_required(VERSION 3.0)
project(main)
set(CMAKE_CXX_STANDARD 17)

add_executable(main interpreter.cpp)
target_compile_options(main PUBLIC -m32)

add_library(Runtime STATIC IMPORTED)
set_target_properties(Runtime PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/runtime/runtime.a
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/runtime/
        INTERFACE_COMPILE_DEFINITIONS "USING_RUNTIME;RUNTIME_STATIC"
)

target_link_libraries(main PRIVATE Runtime)
target_link_options(main PUBLIC -m32)