#pragma once
// odbg_plugin_sdk.h - the plugin SDK for open-debugger (odbg), modeled as
// closely as reasonably possible on OllyDbg's own plugin architecture: the
// host (odbg.exe) exports a flat C API that a plugin DLL links against
// directly (exactly like real Olly plugins link against ollydbg.exe's
// export table), and the plugin DLL in turn exports a small, fixed set of
// lifecycle functions the host resolves by name with GetProcAddress -
// Plugindata / Plugininit / Pluginmenu / Pluginaction / Paused / Pluginclose,
// matching Olly's ODBG_Plugindata / ODBG_Plugininit / ODBG_Pluginmenu /
// ODBG_Pluginaction / ODBG_Paused / ODBG_Pluginclose in name and spirit.
//
// This is NOT binary-compatible with real compiled OllyDbg plugin DLLs -
// Olly's actual ABI depends on hundreds of additional host functions and
// direct access to Olly's internal structs (t_reg, t_module, t_table, ...)
// that were never fully public. This SDK instead gives plugin authors the
// same *shape* of system (host-exported globals, name-based lifecycle
// exports, an origin/menu/action model, a Paused callback handed the
// register file directly) against our own, honestly-documented ABI.
//
// A plugin DLL:
//   #include "odbg_plugin_sdk.h"
//   extern "C" __declspec(dllexport) int Odbg_Plugindata(char shortname[32]) {...}
//   extern "C" __declspec(dllexport) int Odbg_Plugininit(int hostVersion) {...}
//   extern "C" __declspec(dllexport) int Odbg_Pluginmenu(int origin, char items[][32], int maxItems) {...}
//   extern "C" __declspec(dllexport) void Odbg_Pluginaction(int origin, int action) {...}
//   extern "C" __declspec(dllexport) void Odbg_Paused(int reason, const OdbgRegs* regs) {...}
//   extern "C" __declspec(dllexport) void Odbg_Pluginclose(void) {...}
// and links against odbg.lib (generated alongside odbg.exe) to call the
// Odbg_* host functions declared below directly, by name, like Olly plugins
// call Readmemory()/Setreg()/etc. from ollydbg.exe.

#include <windows.h>

#ifdef ODBG_BUILDING_HOST
#define ODBG_API extern "C" __declspec(dllexport)
#else
#define ODBG_API extern "C" __declspec(dllimport)
#endif

#define ODBG_PLUGIN_ABI_VERSION 1

// Menu origins, Olly-style (Olly's PM_MAIN/PM_DISASM/PM_DUMP/...): which
// part of the UI a plugin's menu is being asked for / an action fired from.
// We only have one today; the parameter stays so a real multi-origin system
// (per-pane context menus) can grow in without breaking existing plugins.
enum {
    ODBG_ORIGIN_PLUGINS_MENU = 0,
};

// Reasons passed to Odbg_Paused - why the target is stopped right now.
enum {
    ODBG_PAUSE_BREAKPOINT = 0,
    ODBG_PAUSE_STEP       = 1,
    ODBG_PAUSE_ATTACH_OR_LAUNCH = 2,
};

#pragma pack(push, 1)
struct OdbgRegs {
    unsigned long long rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, rip;
    unsigned long long r8, r9, r10, r11;
};
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Host-exported flat API. Every call not made from the engine's own worker
// thread (i.e. anything called from Odbg_Pluginaction, which the host invokes
// on its UI thread) is transparently marshaled onto the correct thread by
// the host before it touches the debug engine - a plugin author never needs
// to think about this.
// ---------------------------------------------------------------------------

ODBG_API bool  Odbg_SessionActive();
ODBG_API bool  Odbg_Stopped();

ODBG_API bool  Odbg_Readmemory(unsigned long long addr, void* buf, unsigned long size);
ODBG_API bool  Odbg_Writememory(unsigned long long addr, const void* buf, unsigned long size);

ODBG_API unsigned long long Odbg_Getreg(const wchar_t* name);
ODBG_API bool                Odbg_Setreg(const wchar_t* name, unsigned long long value);
ODBG_API void                Odbg_Getregs(OdbgRegs* out);

ODBG_API int   Odbg_Addbreakpoint(const wchar_t* moduleBangSymbolOrHexAddr);
ODBG_API bool  Odbg_Removebreakpoint(int id);

ODBG_API unsigned long long Odbg_Getpeb();

// Go/pause/step - same verbs as the command bar's g/pause/t/p.
ODBG_API void  Odbg_Go();
ODBG_API void  Odbg_Pause();
ODBG_API void  Odbg_Stepinto();
ODBG_API void  Odbg_Stepover();

// Appends a line to the GUI's Log pane, prefixed with the plugin's name.
ODBG_API void  Odbg_Log(const char* text);

// Register a command name + one-line help so it shows up in the debugger's
// Command Reference window (the "?" button on the Command pane), under a
// "Plugin commands" section. Call this from Odbg_Plugininit. It is purely
// informational - listing what your plugin offers to a user reading the help;
// the plugin still does its work through its menu/action handlers.
ODBG_API void  Odbg_RegisterCommand(const char* name, const char* help);

// Persistent per-plugin settings. Anything a plugin stores here is written to
// the host's settings file when odbg exits and is readable again on the next
// run - the same file, and the same lifetime, as the debugger's own options.
// Keys are namespaced per plugin by the host, so two plugins can both use
// "enabled" without colliding. Values are plain text; a plugin that wants a
// number formats and parses it itself.
//
// Odbg_Getsetting fills `buf` with the stored value (empty string if the key
// was never set) and returns its length, or -1 on bad arguments. Call these
// from Odbg_Plugininit (to load) and Odbg_Pluginclose (to store).
ODBG_API void  Odbg_Setsetting(const char* key, const char* value);
ODBG_API int   Odbg_Getsetting(const char* key, char* buf, int bufSize);

// ---------------------------------------------------------------------------
// Plugin-exported lifecycle functions (resolved by the host via
// GetProcAddress; a plugin need not export all of them - Pluginmenu/
// Pluginaction/Paused/Pluginclose are optional).
// ---------------------------------------------------------------------------

typedef int  (*Odbg_PlugindataFn)(char shortname[32]);
typedef int  (*Odbg_PlugininitFn)(int hostVersion);
typedef int  (*Odbg_PluginmenuFn)(int origin, char items[][32], int maxItems);
typedef void (*Odbg_PluginactionFn)(int origin, int action);
typedef void (*Odbg_PausedFn)(int reason, const OdbgRegs* regs);
typedef void (*Odbg_PlugincloseFn)(void);
