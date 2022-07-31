cmake_minimum_required(VERSION 3.21.0)

project("ccache")

set(CMAKE_CXX_STANDARD 17)

set(TARGAT_NAME "ccache")

message("prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

include(FindBoost)

set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

source_group("Header Files" ${HEADER_FILES})
source_group("Source Files" ${SOURCE_FILES})

add_executable(
    ${TARGAT_NAME}
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_include_directories(
    ${TARGAT_NAME}
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "/usr/local/include"
)

target_link_directories(
    ${TARGAT_NAME}
    PRIVATE
    "/usr/local/lib"
)

find_package(Boost COMPONENTS system regex REQUIRED)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find boost!")
endif()

message("boost_LIBS: " ${Boost_LIBRARIES})

# find_package(OpenSSL COMPONENTS crypto REQUIRED)
# if(NOT OpenSSL_FOUND)
#     message(FATAL_ERROR "Could not find openssl!")
# endif()

target_include_directories(
    ${TARGAT_NAME}
    PRIVATE
    ${Boost_INCLUDE_DIRS}
)

find_library(OPENSSL_CRYPTO_LIBS NAMES crypto)
message("crypto_LIBS: " ${OPENSSL_CRYPTO_LIBS})

target_link_libraries(
    ${TARGAT_NAME}
    ${OPENSSL_CRYPTO_LIBS}
    # ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
)