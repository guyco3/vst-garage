cmake_minimum_required(VERSION 3.22)

project(vst-garage VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.3
)

# Don't build JUCE extras/tools by default in CI.
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(juce)

message(STATUS "Configured JUCE from FetchContent")

# Shared code module (optional for now).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/shared-lib/CMakeLists.txt")
  add_subdirectory(plugins/shared-lib)
endif()

# Discover plugin projects automatically.
file(GLOB children RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/plugins" "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")
foreach(child ${children})
  if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/plugins/${child}")
    if(NOT child STREQUAL "shared-lib" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/${child}/CMakeLists.txt")
      message(STATUS "Adding plugin target: ${child}")
      add_subdirectory("plugins/${child}")
    endif()
  endif()
endforeach()
