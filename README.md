# open-debugger plugin SDK

Everything needed to build a plugin for [open-debugger](https://github.com/nikolas-chambers/open-debugger)
(`odbg`), modeled as closely as reasonably possible on OllyDbg's plugin
architecture:

* `plugin_sdk/odbg_plugin_sdk.h` - the host's flat C API (`Odbg_*`) that a
  plugin links against directly, exactly like a real Olly plugin links against
  `ollydbg.exe`, plus the lifecycle exports the host resolves by name
  (`Odbg_Plugindata`, `Odbg_Plugininit`, `Odbg_Pluginmenu`, `Odbg_Pluginaction`,
  `Odbg_Paused`, `Odbg_Pluginclose`).
* `cmake/OdbgPlugin.cmake` - `add_odbg_plugin()`, which compiles one plugin,
  links it to `odbg.lib`, and drops the DLL into the host's `plugins` folder.
* `plugins/odbg-sample_plugin` - a reference plugin that exercises every call
  in the header, with the threading and lifetime rules called out inline.

## Using it in your own plugin repo

Add this repo as a submodule and pull it in:

```cmake
add_subdirectory(external/open-debugger-plugins_sdk)   # defines odbg::plugin_sdk
add_odbg_plugin(my-plugin)                             # builds plugins/my-plugin/my-plugin.cpp
```

`add_odbg_plugin()` needs to find the host's build tree for `odbg.lib`; it
defaults to `${CMAKE_SOURCE_DIR}/../open-debugger/build`, override with
`-DODBG_ROOT=...` or `-DODBG_BUILD_DIR=...`. (When the whole thing is
configured from inside open-debugger - which carries the plugin repos as
submodules - there is no path to point at: the plugin links the `odbg` target
directly and CMake orders the build itself.)

Build open-debugger first, for the same architecture - a plugin DLL is loaded
into `odbg.exe`'s own process.

As a subproject this only defines the interface library and the helper; the
sample plugin is built when this repo is configured on its own.

## Building the sample plugin

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The DLL lands in `../open-debugger/build/Release/plugins`, where `odbg.exe`
picks it up at startup.

---

<table>
<tr><td>

### ☕ Buy me a coffee?

**Venmo · Cash App · PayPal — "NikAndRigatoni" (Nikolas Chambers)**

The honest version: my dog and I are living in the car right now. I spend my
days writing code anyway - bringing old projects of mine back to life one at a
time, and learning everything I can along the way. If anything here was useful
to you, a few bucks goes to dog food, gas, and keeping the laptop running, and
it buys me more hours to keep building. Either way, thanks for reading this far.

</td></tr>
</table>
