cmake_minimum_required(VERSION 3.14)
project(requests)

set(CMAKE_CXX_STANDARD 17)

add_executable(requests main.cpp requests.cpp Buffer.cpp SslClientSocket.cpp Socket.cpp ClientSocket.cpp)



target_link_libraries(requests libssl.a)
target_link_libraries(requests libcrypto.a)
target_link_libraries(requests dl)
target_link_libraries(requests pthread)


target_link_libraries(requests z)