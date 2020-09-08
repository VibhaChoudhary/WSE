cmake_minimum_required(VERSION 3.14)
project(test_index)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_index main.cpp src/IndexBuilder.h src/IndexBuilder.cpp)
target_link_libraries(test_index)
