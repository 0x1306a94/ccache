# get git hash
macro(get_git_hash _git_hash)
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
      OUTPUT_VARIABLE ${_git_hash}
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endmacro()

# get git branch
macro(get_git_branch _git_branch)
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
      OUTPUT_VARIABLE ${_git_branch}
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
endmacro()

function(git_submodule_check check_file)
  find_package(Git QUIET)
  if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
      message(STATUS "Submodule update")
      execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMOD_RESULT)
      if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(
          FATAL_ERROR
            "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules"
        )
      endif()
    endif()
  endif()

  if(NOT EXISTS ${check_file})
    message(STATUS "Checking ${check_file}")
    message(
      FATAL_ERROR
        "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again."
    )
  endif()
endfunction()

macro(find_homebrew_openssl)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND brew --prefix openssl
      RESULT_VARIABLE BREW_OPENSSL
      OUTPUT_VARIABLE BREW_OPENSSL_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(BREW_OPENSSL EQUAL 0 AND EXISTS "${BREW_OPENSSL_PREFIX}")
      message(
        STATUS
          "Found OpenSSL keg installed by Homebrew at ${BREW_OPENSSL_PREFIX}")
      set(OPENSSL_ROOT_DIR "${BREW_OPENSSL_PREFIX}/")
      set(OPENSSL_INCLUDE_DIR "${BREW_OPENSSL_PREFIX}/include")
      set(OPENSSL_LIBRARIES "${BREW_OPENSSL_PREFIX}/lib")
      # set(OPENSSL_CRYPTO_LIBRARY "${BREW_OPENSSL_PREFIX}/lib/libcrypto.dylib")
      # set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
  endif()
endmacro()

macro(find_homebrew_icu4c)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND brew --prefix icu4c
      RESULT_VARIABLE BREW_ICU4C
      OUTPUT_VARIABLE BREW_ICU4C_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(BREW_ICU4C EQUAL 0 AND EXISTS "${BREW_ICU4C_PREFIX}")
      message(
        STATUS "Found icu4c keg installed by Homebrew at ${BREW_ICU4C_PREFIX}")
      set(BREW_ICU4C_ROOT_DIR "${BREW_BREW_ICU4C_PREFIX}/")
      set(BREW_ICU4C_INCLUDE_DIR "${BREW_BREW_ICU4C_PREFIX}/include")
      set(BREW_ICU4C_LIBRARIES "${BREW_BREW_ICU4C_PREFIX}/lib")
    endif()
  endif()
endmacro()

macro(find_homebrew_xxhash)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND brew --prefix xxhash
      RESULT_VARIABLE BREW_XXHASH
      OUTPUT_VARIABLE BREW_XXHASH_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(BREW_XXHASH EQUAL 0 AND EXISTS "${BREW_XXHASH_PREFIX}")
      message(
        STATUS "Found xxhash keg installed by Homebrew at ${BREW_XXHASH_PREFIX}"
      )
      set(XXHASH_ROOT_DIR "${BREW_XXHASH_PREFIX}/")
      set(XXHASH_INCLUDE_DIR "${BREW_XXHASH_PREFIX}/include")
      set(XXHASH_LIBRARIES "${BREW_XXHASH_PREFIX}/lib")
      # set(XXHASH_LIBRARY "${BREW_XXHASH_PREFIX}/lib/libxxhash.a")
      # set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
  endif()
endmacro()
