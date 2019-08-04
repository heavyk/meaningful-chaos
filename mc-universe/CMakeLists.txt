cmake_minimum_required (VERSION 3.0)

project (MC-Universe)

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

# https://stackoverflow.com/questions/21163188/most-simple-but-complete-cmake-example
file(GLOB_RECURSE sources *.c *.cpp *.h *.hpp)

add_executable(mc-universe mc-universe.cpp)
# add_library(mc-core mc-universe.cpp)
target_include_directories(mc-universe PRIVATE external/Simple-WebSocket-Server)

find_package(Threads REQUIRED)
target_link_libraries(mc-universe PRIVATE ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost 1.54.0 COMPONENTS system thread coroutine context REQUIRED)
target_link_libraries(mc-universe PRIVATE ${Boost_LIBRARIES})
target_include_directories(mc-universe SYSTEM PRIVATE ${Boost_INCLUDE_DIR})
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    target_compile_definitions(mc-universe PRIVATE USE_BOOST_REGEX)
    find_package(Boost 1.54.0 COMPONENTS regex REQUIRED)
    target_link_libraries(mc-universe PRIVATE ${Boost_LIBRARIES})
    target_include_directories(mc-universe SYSTEM PRIVATE ${Boost_INCLUDE_DIR})
endif()

if(WIN32)
    target_link_libraries(mc-universe INTERFACE ws2_32 wsock32)
endif()


if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()
find_package(OpenSSL REQUIRED)
target_link_libraries(mc-universe PRIVATE ${OPENSSL_LIBRARIES})
target_include_directories(mc-universe SYSTEM PRIVATE ${OPENSSL_INCLUDE_DIR})

# message("openssl root dir: ${OPENSSL_ROOT_DIR}")
# message("openssl include dir: ${OPENSSL_INCLUDE_DIR}")
# message("openssl libraries: ${OPENSSL_LIBRARIES}")

set(BUILD_TESTING ON)


if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(external/Catch2)
    add_compile_options(-DBUILD_TESTING)
    add_subdirectory(test)
endif()
