cmake_minimum_required(VERSION 3.16)
project(MidnightReignCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mr_core INTERFACE)
target_include_directories(mr_core INTERFACE include)

enable_testing()
add_executable(mr_core_tests tests/core_tests.cpp)
target_link_libraries(mr_core_tests PRIVATE mr_core)
if(MSVC)
  target_compile_options(mr_core_tests PRIVATE /W4 /WX)
else()
  target_compile_options(mr_core_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME mr_core_tests COMMAND mr_core_tests)
