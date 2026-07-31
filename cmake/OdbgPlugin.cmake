# OdbgPlugin.cmake - the build half of the odbg plugin SDK.
#
# Include this (it comes with the SDK, and add_subdirectory of the SDK does it
# for you) and you get add_odbg_plugin(), which builds one plugin DLL the way
# odbg.exe expects to find it:
#
#   * compiled against plugin_sdk/odbg_plugin_sdk.h (via odbg::plugin_sdk)
#   * linked against odbg.exe's import library, exactly like a real OllyDbg
#     plugin links against ollydbg.exe
#   * dropped into the host's <build>/<config>/plugins folder, which is the
#     directory odbg.exe scans for *.dll at startup
#
# Because the plugin links the host's exports, the host has to be built first,
# and it must be built for the same architecture - a plugin DLL is loaded into
# odbg.exe's own process, so configure your plugin project with the same -A.

if(NOT DEFINED ODBG_ROOT)
    set(ODBG_ROOT "${CMAKE_SOURCE_DIR}/../open-debugger"
        CACHE PATH "Root of the open-debugger project that hosts these plugins")
endif()
set(ODBG_BUILD_DIR "${ODBG_ROOT}/build"
    CACHE PATH "open-debugger's configured build tree (holds <config>/odbg.lib)")

if(NOT EXISTS "${ODBG_BUILD_DIR}")
    message(WARNING
        "ODBG_BUILD_DIR does not exist yet: ${ODBG_BUILD_DIR}\n"
        "Build open-debugger first - plugins link against the odbg.lib it produces.")
endif()

function(add_odbg_plugin name)
    add_library(${name} SHARED plugins/${name}/${name}.cpp)
    target_link_libraries(${name} PRIVATE odbg::plugin_sdk)

    if(TARGET odbg)
        # Being configured from inside the host's own build (open-debugger
        # carries the plugin repos as submodules). Link the target itself, so
        # CMake orders the build and follows the exe wherever it lands.
        target_link_libraries(${name} PRIVATE odbg)
        set(out_dir "$<TARGET_FILE_DIR:odbg>/plugins")
    else()
        # Standalone plugin repo: the host is a separate, already-built tree,
        # so link its import library by path the way any out-of-tree plugin
        # would and drop the DLL into the plugins folder it scans.
        target_link_libraries(${name} PRIVATE "${ODBG_BUILD_DIR}/$<CONFIG>/odbg.lib")
        set(out_dir "${ODBG_BUILD_DIR}/$<CONFIG>/plugins")
    endif()

    if(MSVC)
        target_compile_options(${name} PRIVATE /W3 /EHsc)
    endif()
    # A generator expression in the output directory suppresses the per-config
    # subfolder MSBuild would otherwise append, so the DLL lands exactly here.
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${out_dir}"
        LIBRARY_OUTPUT_DIRECTORY "${out_dir}")
endfunction()
