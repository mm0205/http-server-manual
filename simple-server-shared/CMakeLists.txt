project(simple-server-shared)

add_library(
        ${PROJECT_NAME}
        http_request_t.h
        http_request_t.cpp
        http_response_t.cpp
        http_response_t.h
        http_server_t.cpp
        http_server_t.h
        http_constants_t.cpp http_constants_t.h)

find_package(Boost 1.72.0 REQUIRED)
if(Boost_FOUND)
    target_include_directories(
            ${PROJECT_NAME}
            PRIVATE
            ${Boost_INCLUDE_DIRS}
    )
endif()