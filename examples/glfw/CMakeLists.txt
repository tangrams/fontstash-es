cmake_minimum_required(VERSION 2.8)
project(fontstash_es)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x -stdlib=libc++ -g -O0")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

if(NOT GLFW_FOUND)
    message(SEND_ERROR "GLFW not found")
    return()
else()
    include_directories(${GLFW_INCLUDE_DIRS})
    message(STATUS "Found GLFW ${GLFW_PREFIX}")
endif()

list(APPEND GLFW_LDFLAGS
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreFoundation"
    "-framework CoreVideo")

include_directories(../../fontstash)

add_executable(glfw.out main.cpp)
add_executable(particles.out particles.cpp)
add_executable(colors.out colors.cpp)

target_link_libraries(glfw.out ${GLFW_LDFLAGS})
target_link_libraries(particles.out ${GLFW_LDFLAGS})
target_link_libraries(colors.out ${GLFW_LDFLAGS})
