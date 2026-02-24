cmake_minimum_required(VERSION 3.22)

project(VSTGarage VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE must be added before any plugin targets
add_subdirectory(libs/JUCE)

# Plugins
add_subdirectory(plugins/vst/NewProject)
