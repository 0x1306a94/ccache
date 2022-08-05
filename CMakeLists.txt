cmake_minimum_required(VERSION 3.20.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/utils.cmake)

project("ccache")

set(CMAKE_CXX_STANDARD 17)

set(CCACHE_TARGET_NAME "ccache")

string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
message("Building date ${BUILD_DATE}")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)
message("Building version ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# 获取当前的GIT_HASH
set(GIT_COMMIT "")
get_git_hash(GIT_COMMIT)
message(STATUS "Git commit is ${GIT_COMMIT}")

# 获取当前的分支
set(GIT_BRANCH "")
get_git_branch(GIT_BRANCH)
message(STATUS "Git branch is ${GIT_BRANCH}")

option(USE_CACAHE_KEY_OPENSSL_MD5 "Use OpenSSL MD5 for ccache key" ON)
option(USE_CACAHE_KEY_XXHASH "Use xxHash for ccache key" OFF)
option(MTR_ENABLED "Enable mtr" OFF)

git_submodule_check("${PROJECT_SOURCE_DIR}/third_party/yaml-cpp/CMakeLists.txt")

set(YAML_CPP_BUILD_TOOLS
    OFF
    CACHE BOOL "Build tools for yaml-cpp" FORCE)
set(YAML_BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "Build shared libraries for yaml-cpp" FORCE)
add_subdirectory(third_party/yaml-cpp)
add_subdirectory(third_party/fmt)

# message("lib prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")

# message("lib suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/configure/configure.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

source_group("Header Files" ${HEADER_FILES})
source_group("Source Files" ${SOURCE_FILES})

add_executable(${CCACHE_TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(${CCACHE_TARGET_NAME} yaml-cpp)
target_link_libraries(${CCACHE_TARGET_NAME} fmt)

# set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

# set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")

target_include_directories(${CCACHE_TARGET_NAME}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(
  ${CCACHE_TARGET_NAME}
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/yaml-cpp/include")
target_include_directories(
  ${CCACHE_TARGET_NAME}
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include")
target_include_directories(${CCACHE_TARGET_NAME}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# boost
find_package(
  Boost
  COMPONENTS system regex filesystem log
  REQUIRED)
if(Boost_FOUND)
  message(STATUS "Found Boost ${Boost_VERSION}")
  message("boost_LIBS: " ${Boost_LIBRARIES})
  # Add the include directories for compiling
  target_include_directories(${CCACHE_TARGET_NAME} PUBLIC ${BOOST_INCLUDE_DIR})
  target_link_libraries(${CCACHE_TARGET_NAME} ${Boost_LIBRARIES})
else()
  message(FATAL_ERROR "Boost Not Found")
endif()

if(USE_CACAHE_KEY_OPENSSL_MD5)

  find_homebrew_openssl()
  find_package(OpenSSL REQUIRED)
  if(OPENSSL_FOUND)
    message("openssl_LIBS: " ${OPENSSL_CRYPTO_LIBRARY})
    # Add the include directories for compiling
    target_include_directories(${CCACHE_TARGET_NAME}
                               PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${CCACHE_TARGET_NAME} ${OPENSSL_CRYPTO_LIBRARY})
    message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")
  else()
    message(FATAL_ERROR "OpenSSL Not Found")
  endif()

elseif(USE_CACAHE_KEY_XXHASH)

  find_homebrew_xxhash()

  find_library(XXHASH_LIBRARY NAMES xxhash REQUIRED)
  if(XXHASH_LIBRARY)
    message("xxhash_LIBS: " ${XXHASH_LIBRARY})
    # Add the include directories for compiling
    target_include_directories(${CCACHE_TARGET_NAME}
                               PUBLIC ${XXHASH_INCLUDE_DIR})
    target_link_libraries(${CCACHE_TARGET_NAME} ${XXHASH_LIBRARY})
    message(STATUS "Found xxhash")
  else()
    message(FATAL_ERROR "xxhash Not Found")
  endif()

endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  # set(CMAKE_OSX_ARCHITECTURES arm64;x86_64) message(STATUS "archs:
  set(CCACHE_MACOSX_PLATFORM 1)
endif()

if(MTR_ENABLED)
  add_subdirectory(third_party/minitrace)
  target_link_libraries(${CCACHE_TARGET_NAME} minitrace)
  target_compile_definitions(minitrace
                             PUBLIC "-DMTR_COPY_EVENT_CATEGORY_AND_NAME")
endif()

configure_file("${PROJECT_SOURCE_DIR}/configure/configure.h.in"
               "${PROJECT_SOURCE_DIR}/configure/configure.h" @ONLY)

target_include_directories(${CCACHE_TARGET_NAME}
                           PUBLIC "${PROJECT_SOURCE_DIR}/configure")
