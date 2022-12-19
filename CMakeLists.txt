cmake_minimum_required(VERSION 3.12.0)
project(ASIO_ECHO_SERVER)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

add_link_options(-fsanitize=address)

find_package(Threads)

add_executable(server_single server_single_threaded.cpp)
add_executable(server_one_to_one server_one_to_one.cpp)

add_executable(server_thread_pool server_thread_pool.cpp)
target_link_libraries(server_thread_pool ${CMAKE_THREAD_LIBS_INIT})

add_executable(client client.cpp)