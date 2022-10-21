cmake_minimum_required(VERSION 3.12.0)
project(ASIO_ECHO_SERVER)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_link_options(-fsanitize=address)

add_executable(server_single server_single_threaded.cpp)
add_executable(server_multi server_multi_threaded.cpp)
add_executable(client client.cpp)