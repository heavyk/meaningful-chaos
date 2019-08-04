# make the parent directory path available for includes
include_directories(..)

# to fake out cmake, the test directory is made with symlinks from *.hpp -> *.cpp

add_executable(Grid-test Grid.cpp)
add_test(Grid-test Grid-test)
target_link_libraries(Grid-test Catch2::Catch2)

# TODO make this automatic, with a node script
