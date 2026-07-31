cmake_minimum_required(VERSION 3.20)
project(open_debugger_plugins_sdk CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The odbg plugin SDK: the header a plugin compiles against
# (plugin_sdk/odbg_plugin_sdk.h), the add_odbg_plugin() helper that builds one
# the way the host expects to load it (cmake/OdbgPlugin.cmake), and
# odbg-sample_plugin - the reference plugin that exercises every call in the
# header.
#
# Two ways to use this project:
#
#   Standalone - configure it directly and it builds odbg-sample_plugin into
#   the host's plugins folder, same as any other plugin project.
#
#   As a subproject - a plugin repo carries this as a git submodule and does
#   add_subdirectory(external/open-debugger-plugins_sdk). That defines the
#   odbg::plugin_sdk interface library and add_odbg_plugin(), and deliberately
#   does NOT build the sample plugin - a consumer wants the SDK, not our demo.

add_library(odbg_plugin_sdk INTERFACE)
add_library(odbg::plugin_sdk ALIAS odbg_plugin_sdk)
target_include_directories(odbg_plugin_sdk INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/plugin_sdk")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(OdbgPlugin)

# (spelled out rather than PROJECT_IS_TOP_LEVEL so 3.20 stays enough)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_odbg_plugin(odbg-sample_plugin)
endif()
