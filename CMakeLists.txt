cmake_minimum_required(VERSION 3.0.0)
project(reeee VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

add_executable(reeee main.cpp triangle.cpp quickAndDirty.cpp objLoader.cpp)
target_link_libraries(reeee glfw vulkan dl pthread X11 Xxf86vm Xrandr Xi)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O3")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
