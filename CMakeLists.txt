cmake_minimum_required(VERSION 3.15)
project(http_server)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(simple-server-shared)
add_subdirectory(simple-server-01)
add_subdirectory(simple-server-02-cgi)
add_subdirectory(simple-server-03-event-driven)
add_subdirectory(simple-server-04-mod_lua)
