cmake_minimum_required(VERSION 3.10)
project(libpow LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)

if (NOT MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# Tests are built only when libpow is the top level project: a parent that pulls
# it in with add_subdirectory gets the library alone
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(LIBPOW_IS_TOP_LEVEL ON)
else()
  set(LIBPOW_IS_TOP_LEVEL OFF)
endif()
option(LIBPOW_BUILD_TESTS "Build libpow tests" ${LIBPOW_IS_TOP_LEVEL})

# --- CPU architecture check ---
# Ensures all enabled ISA options belong to the same architecture.
set(LIBPOW_CPU_ARCH "" CACHE INTERNAL "")
function(libpow_require_arch arch)
  if (LIBPOW_CPU_ARCH STREQUAL "")
    set(LIBPOW_CPU_ARCH "${arch}" CACHE INTERNAL "" FORCE)
  elseif (NOT LIBPOW_CPU_ARCH STREQUAL "${arch}")
    message(FATAL_ERROR
      "libpow: conflicting CPU architecture: already set to '${LIBPOW_CPU_ARCH}', "
      "but option requires '${arch}'")
  endif()
endfunction()

# --- Target architecture (e.g. native, znver4, armv8-a+crypto) ---
set(LIBPOW_MARCH "" CACHE STRING "Value for -march flag (e.g. native, znver4, alderlake)")
if (LIBPOW_MARCH)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${LIBPOW_MARCH}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${LIBPOW_MARCH}")
  message(STATUS "libpow: -march=${LIBPOW_MARCH}")
endif()

# --- ISA extension options ---
option(LIBPOW_SHA_NI     "Enable SHA-NI (x86 SHA extensions)" OFF)
option(LIBPOW_ARM_CRYPTO "Enable ARM crypto extensions (SHA-256, AES)" OFF)

add_library(pow
  sha256.c
)

target_include_directories(pow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (LIBPOW_SHA_NI)
  libpow_require_arch(x86_64)
  target_compile_definitions(pow PRIVATE LIBPOW_SHANI_ENABLED)
  target_compile_options(pow PRIVATE -msha -msse4.1)
endif()

if (LIBPOW_ARM_CRYPTO)
  libpow_require_arch(arm)
  target_compile_definitions(pow PRIVATE LIBPOW_ARM_SHA2_ENABLED)
  target_compile_options(pow PRIVATE -march=armv8-a+crypto)
endif()

# Test executable

if (LIBPOW_BUILD_TESTS)
  add_executable(libpow_test
    test/main.cpp
    test/baseBlobTest.cpp
    test/sha256Test.cpp
    test/uintTest.cpp
    test/uintOpsTest.cpp
  )

  target_link_libraries(libpow_test pow)
endif()
