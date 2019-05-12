cmake_minimum_required(VERSION 3.14)
project(requests)

set(CMAKE_CXX_STANDARD 17)

add_executable(requests main.cpp requests.cpp Buffer.cpp SslClientSocket.cpp Socket.cpp ClientSocket.cpp)
add_executable(test test.cpp)


target_link_libraries(requests ssl)
target_link_libraries(requests crypto)
target_link_libraries(requests dl)
target_link_libraries(requests pthread)


target_link_libraries(requests z)