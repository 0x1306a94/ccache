cmake_minimum_required(VERSION 3.21.0)

project("ccache")

set(CMAKE_CXX_STANDARD 17)

set(TARGAT_NAME "ccache")

message("lib prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
message("lib suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

source_group("Header Files" ${HEADER_FILES})
source_group("Source Files" ${SOURCE_FILES})

add_executable(${TARGAT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

# set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")

target_include_directories(${TARGAT_NAME}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# icu4c
# if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
#   execute_process(
#     COMMAND brew --prefix icu4c
#     RESULT_VARIABLE BREW_ICU4C
#     OUTPUT_VARIABLE BREW_ICU4C_PREFIX
#     OUTPUT_STRIP_TRAILING_WHITESPACE)
#   if(BREW_ICU4C EQUAL 0 AND EXISTS "${BREW_ICU4C_PREFIX}")
#     message(
#       STATUS "Found icu4 keg installed by Homebrew at ${BREW_ICU4C_PREFIX}")
#     set(ICU4C_ROOT_DIR "${BREW_ICU4C_PREFIX}/")
#     set(ICU4C_INCLUDE_DIR "${BREW_ICU4C_PREFIX}/include")
#     set(ICU4C_LIBRARIES "${BREW_ICU4C_PREFIX}/lib")

#     target_link_directories(${TARGAT_NAME} PUBLIC ${ICU4C_LIBRARIES})
#   endif()
# endif()

# boost
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  include(FindBoost)
  # set(Boost_USE_STATIC_LIBS ON)
  execute_process(
    COMMAND brew --prefix boost
    RESULT_VARIABLE BREW_BOOST
    OUTPUT_VARIABLE BREW_BOOST_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(BREW_BOOST EQUAL 0 AND EXISTS "${BREW_BOOST_PREFIX}")
    message(
      STATUS "Found Boost keg installed by Homebrew at ${BREW_BOOST_PREFIX}")
    set(BOOST_ROOT_DIR "${BREW_BOOST_PREFIX}/")
    set(BOOST_INCLUDE_DIR "${BREW_BOOST_PREFIX}/include")
    set(BOOST_LIBRARIES "${BREW_BOOST_PREFIX}/lib")
  endif()
endif()

find_package(
  Boost
  COMPONENTS system regex filesystem
  REQUIRED)
if(Boost_FOUND)
  message(STATUS "Found Boost ${Boost_VERSION}")
  message("boost_LIBS: " ${Boost_LIBRARIES})
  # Add the include directories for compiling
  target_include_directories(${TARGAT_NAME} PUBLIC ${BOOST_INCLUDE_DIR})
  # Add the static lib for linking target_link_directories(${TARGAT_NAME} PUBLIC
  # ${BOOST_LIBRARIES})
  target_link_libraries(${TARGAT_NAME} ${Boost_LIBRARIES})
else()
  message(FATAL_ERROR "Boost Not Found")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  execute_process(
    COMMAND brew --prefix OpenSSL
    RESULT_VARIABLE BREW_OPENSSL
    OUTPUT_VARIABLE BREW_OPENSSL_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(BREW_OPENSSL EQUAL 0 AND EXISTS "${BREW_OPENSSL_PREFIX}")
    message(
      STATUS "Found OpenSSL keg installed by Homebrew at ${BREW_OPENSSL_PREFIX}"
    )
    set(OPENSSL_ROOT_DIR "${BREW_OPENSSL_PREFIX}/")
    set(OPENSSL_INCLUDE_DIR "${BREW_OPENSSL_PREFIX}/include")
    set(OPENSSL_LIBRARIES "${BREW_OPENSSL_PREFIX}/lib")
    # set(OPENSSL_CRYPTO_LIBRARY "${BREW_OPENSSL_PREFIX}/lib/libcrypto.dylib")
    # set(OPENSSL_USE_STATIC_LIBS TRUE)
  endif()
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
  message("openssl_LIBS: " ${OPENSSL_CRYPTO_LIBRARY})
  # Add the include directories for compiling
  target_include_directories(${TARGAT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
  # Add the static lib for linking target_link_directories(${TARGAT_NAME} PUBLIC
  # ${OPENSSL_LIBRARIES})
  target_link_libraries(${TARGAT_NAME} ${OPENSSL_CRYPTO_LIBRARY})
  message(STATUS "Found OpenSSL ${OPENSSL_VERSION}")
else()
  message(FATAL_ERROR "OpenSSL Not Found")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  # set(CMAKE_OSX_ARCHITECTURES arm64;x86_64) message(STATUS "archs:
  # ${CMAKE_OSX_ARCHITECTURES}")
  target_compile_options(${TARGAT_NAME} PUBLIC "-DCCACHE_MACOSX_PLATFORM=1")
endif()
