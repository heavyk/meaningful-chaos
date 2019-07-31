# to fake out cmake, the test directory is made with symlinks from *.hpp -> *.cpp

add_executable(Grid-test Grid.cpp)
add_test(Grid-test Grid-test)

# TODO make this automatic, with a node script
