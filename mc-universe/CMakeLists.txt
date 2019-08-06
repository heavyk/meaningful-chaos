cmake_minimum_required (VERSION 3.6)

project (meaningful-chaos)

option(USE_DOUBLE "set ON to double size vectors" OFF)
option(BUILD_TESTING "set ON to build tests" OFF)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra) # -Wsign-conversion
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wthread-safety)
    endif()
else()
    add_compile_options(/W1)
endif()

set(CMAKE_CXX_STANDARD 17)

# add_subdirectory(src/)
include_directories(src/)
include_directories(external/)
add_subdirectory(external/Simple-WebSocket-Server/)
include_directories(external/Simple-WebSocket-Server/)

# https://stackoverflow.com/questions/21163188/most-simple-but-complete-cmake-example
file(GLOB_RECURSE sources *.c *.cpp *.h *.hpp)

# add_library(mc-core ${sources})
add_library(mc-core INTERFACE)
target_include_directories(mc-core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# set_target_properties(mc-core PROPERTIES LINKER_LANGUAGE CXX)

# target_include_directories(mc-core INTERFACE .)
# target_include_directories(mc-core INTERFACE ../external/Simple-WebSocket-Server/)

find_package(Threads REQUIRED)
target_link_libraries(mc-core INTERFACE ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost 1.54.0 COMPONENTS system thread coroutine context REQUIRED)
target_link_libraries(mc-core INTERFACE ${Boost_LIBRARIES})
target_include_directories(mc-core SYSTEM INTERFACE ${Boost_INCLUDE_DIR})
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    target_compile_definitions(mc-core INTERFACE USE_BOOST_REGEX)
    find_package(Boost 1.54.0 COMPONENTS regex REQUIRED)
    target_link_libraries(mc-core INTERFACE ${Boost_LIBRARIES})
    target_include_directories(mc-core SYSTEM PRIVATE ${Boost_INCLUDE_DIR})
endif()

if(WIN32)
    target_link_libraries(mc-core INTERFACE ws2_32 wsock32)
endif()


if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()
find_package(OpenSSL REQUIRED)
target_link_libraries(mc-core INTERFACE ${OPENSSL_LIBRARIES})
target_include_directories(mc-core SYSTEM INTERFACE ${OPENSSL_INCLUDE_DIR})

# message("openssl root dir: ${OPENSSL_ROOT_DIR}")
# message("openssl include dir: ${OPENSSL_INCLUDE_DIR}")
# message("openssl libraries: ${OPENSSL_LIBRARIES}")

# ===========

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    add_executable(mc-universe mc-universe.cpp)
    target_link_libraries(mc-universe mc-core)

    set(BUILD_TESTING ON)
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(external/Catch2/)
    add_compile_options(-DBUILD_TESTING)
    add_subdirectory(test/)
endif()
