project(simple-server-01)



add_executable(
        ${PROJECT_NAME}
        simple-server-01.cpp
        )

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ../simple-server-shared
)

find_package(Boost 1.72.0 REQUIRED)
if(Boost_FOUND)
    target_include_directories(
            ${PROJECT_NAME}
            PRIVATE
            ${Boost_INCLUDE_DIRS}
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE simple-server-shared)