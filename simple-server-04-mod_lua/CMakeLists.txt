project(simple-server-04-mod_lua)

add_executable(${PROJECT_NAME}
        simple-server-04.cpp)

find_package(Boost COMPONENTS filesystem REQUIRED)
if(Boost_FOUND)
    target_include_directories(
            ${PROJECT_NAME}
            PRIVATE
            ${Boost_INCLUDE_DIRS}
    )
endif()

find_package(lua REQUIRED)


target_include_directories(${PROJECT_NAME}
        PRIVATE
        ../simple-server-shared
        ${LUA_INCLUDE_DIR}
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        simple-server-shared
        ${Boost_FILESYSTEM_LIBRARY}
        ${LUA_LIBRARIES}
)
