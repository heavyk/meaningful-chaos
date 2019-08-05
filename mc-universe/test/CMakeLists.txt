# make the parent directory path available for includes
include_directories(../src)

if(NOT MSVC)
    add_compile_options(-fno-access-control)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-thread-safety)
    endif()
endif()

# to fake out cmake, the test directory is made with symlinks from *.hpp -> *.cpp

add_executable(Grid-test Grid.cpp)
add_test(Grid-test Grid-test)
target_link_libraries(Grid-test mc-core)
target_link_libraries(Grid-test Catch2::Catch2)

# add_executable(WsServer-test WsServer.cpp)
# add_test(WsServer-test WsServer-test)
# target_link_libraries(WsServer-test mc-core)
# target_link_libraries(WsServer-test Catch2::Catch2)

# add_executable(mc-universe-test mc-universe.cpp)
# add_test(mc-universe-test mc-universe-test)
# target_link_libraries(mc-universe-test Catch2::Catch2)
# # add_dependencies(mc-universe-test mc-core)
# # target_link_libraries(mc-universe-test mc-core)

# TODO make this automatic, with a node script
