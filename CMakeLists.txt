cmake_minimum_required(VERSION 3.0)
project(SERVER)

set(CMAKE_CXX_STANDARD 17)

add_executable(server main.cpp Server.cpp FileDescriptor.cpp ClientFileDescriptor.cpp EpollController.cpp Utils.cpp)