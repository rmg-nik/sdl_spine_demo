# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)

set (CMAKE_CXX_STANDARD 17)

project ("spine_demo")


# Include sub-projects.
include_directories(${CMAKE_SOURCE_DIR})
include_directories("ThirdParty")
include_directories("ThirdParty/spine-runtimes/spine-c/spine-c/include")

add_definitions(-DHAVE_LIBC)

add_subdirectory ("ThirdParty/SDL")
add_subdirectory ("ThirdParty/spine-runtimes/spine-c")
add_subdirectory ("demo")
