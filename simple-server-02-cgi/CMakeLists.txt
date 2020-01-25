project(simple-server-02-cgi)

add_executable(${PROJECT_NAME}
        simple-server-02-cgi.cpp)

find_package(Boost COMPONENTS filesystem REQUIRED)
if(Boost_FOUND)
    target_include_directories(
            ${PROJECT_NAME}
            PRIVATE
            ${Boost_INCLUDE_DIRS}
    )
endif()

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ../simple-server-shared
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        simple-server-shared
        ${Boost_FILESYSTEM_LIBRARY}
)