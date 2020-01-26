project(simple-server-03-event-driven)

add_executable(
        ${PROJECT_NAME}
        simple-server-03.cpp
)

find_package(Boost COMPONENTS system REQUIRED)
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
        ${Boost_SYSTEM_LIBRARY}
)