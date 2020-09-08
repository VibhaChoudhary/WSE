cmake_minimum_required(VERSION 3.14)
project(QueryProcessor)

set(CMAKE_CXX_STANDARD 17)

add_executable(QueryProcessor main.cpp src/QueryProcessor.cpp src/QueryProcessor.h)
target_link_libraries(QueryProcessor)
