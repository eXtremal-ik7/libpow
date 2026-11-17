cmake_minimum_required(VERSION 3.10)
project(libpow LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)

if (NOT MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Test executable

add_executable(libpow_test
  test/main.cpp
  test/baseBlobTest.cpp
  test/uintTest.cpp
  test/uintOpsTest.cpp
)
