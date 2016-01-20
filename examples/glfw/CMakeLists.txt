cmake_minimum_required(VERSION 2.8)
project(fontstash_es)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x -stdlib=libc++ -g -O0")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x -g -O0")
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

if(NOT GLFW_FOUND)
    message(SEND_ERROR "GLFW not found")
    return()
else()
    include_directories(${GLFW_INCLUDE_DIRS})
    message(STATUS "Found GLFW ${GLFW_PREFIX}")
endif()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
list(APPEND GLFW_LDFLAGS
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreFoundation"
    "-framework CoreVideo")
else()
    find_package(OpenGL REQUIRED)
endif()

include_directories(../../fontstash)

add_executable(glfw.out main.cpp)
add_executable(particles.out particles.cpp)
add_executable(colors.out colors.cpp)

target_link_libraries(glfw.out ${GLFW_LDFLAGS} ${OPENGL_LIBRARIES})
target_link_libraries(particles.out ${GLFW_LDFLAGS} ${OPENGL_LIBRARIES})
target_link_libraries(colors.out ${GLFW_LDFLAGS} ${OPENGL_LIBRARIES})

add_custom_target(copy_resources
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../ios/FontstashiOS/resources
    ${CMAKE_BINARY_DIR}
  # Make resources show up in IDE, at least this should do it
  SOURCES ${RESOURCE_FILES})

add_dependencies(glfw.out copy_resources)
