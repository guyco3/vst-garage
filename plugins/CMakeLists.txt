cmake_minimum_required(VERSION 3.22)
project(VST_GARAGE_PLUGINS VERSION 0.0.1)

# 1. Include JUCE
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libs/JUCE libs/JUCE)

# 2. Add your individual plugins
# add_subdirectory(PluginA)
# add_subdirectory(PluginB)