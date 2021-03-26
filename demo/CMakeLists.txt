cmake_minimum_required (VERSION 3.8)

set(targetName "demo")

set(projectSources "./main.cpp" "./sdl_spine.cpp" "./sdl_spine.h")

if(ANDROID)
  # On android the final binary is a shared library not an executable.
  add_library(${targetName} SHARED ${projectSources})
else()
  add_executable(${targetName} ${projectSources})
endif()

add_definitions(-DSTB_IMAGE_IMPLEMENTATION)

target_link_libraries(${targetName} spine-c SDL2main SDL2-static)

add_custom_command(TARGET ${targetName} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:${targetName}>/data)
