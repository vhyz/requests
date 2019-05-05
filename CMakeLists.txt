cmake_minimum_required(VERSION 3.14)
project(requests)

set(CMAKE_CXX_STANDARD 17)

add_executable(requests main.cpp requests.cpp rio.cpp)

target_link_libraries(requests libz.a)