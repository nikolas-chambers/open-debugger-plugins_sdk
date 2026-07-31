// open-sample - a reference plugin that exercises every function in the odbg
// plugin SDK. It is deliberately not "useful" as a tool; it exists so a plugin
// author can see, in one file, how each lifecycle export and each host-exported
// Odbg_* call is meant to be used, with the threading and lifetime rules called
// out inline.
//
// The six lifecycle exports the host resolves by name (all optional except
// Plugindata/Plugininit):
//   Odbg_Plugindata   - name yourself + declare the ABI you built against
//   Odbg_Plugininit   - accept/reject the host version; register commands
//   Odbg_Pluginmenu   - contribute Plugins-menu items
//   Odbg_Pluginaction - handle a menu item being picked (runs on the UI thread;
//                       host-exported calls marshal to the engine for you)
//   Odbg_Paused       - called every time the target stops (breakpoint/step/
//                       launch); handed the register file directly
//   Odbg_Pluginclose  - tear-down
//
// Every Odbg_* host call below is safe to make from Odbg_Pluginaction and
// Odbg_Paused; the host marshals onto the engine's worker thread as needed.

#include "odbg_plugin_sdk.h"

#include <cstdio>
#include <cstring>

// Menu item indices - kept in an enum so Pluginmenu and Pluginaction agree.
enum {
    kMenuDumpState = 0,   // read regs/memory/PEB and log a summary
    kMenuSetRax,          // demonstrate writing a register
    kMenuStep,            // drive execution (step into)
    kMenuGo,              // resume
    kMenuBreakMain,       // add a breakpoint by module!symbol
    kMenuToggleChatty,    // demonstrate a persisted plugin setting
    kMenuCount
};

static int g_lastBpId = -1;

// A setting of our own, loaded in Plugininit and stored in Pluginclose. The
// host namespaces the key by plugin name and keeps it in odbg_settings.ini, so
// it comes back on the next run exactly like the debugger's own options.
static bool g_chatty = true;

extern "C" __declspec(dllexport) int Odbg_Plugindata(char shortname[32]) {
    // Name shown in the Plugins menu. Return the ABI version you compiled
    // against so the host can reject a mismatched plugin.
    strcpy_s(shortname, 32, "Sample (SDK demo)");
    return ODBG_PLUGIN_ABI_VERSION;
}

extern "C" __declspec(dllexport) int Odbg_Plugininit(int hostVersion) {
    // Refuse to load against a host ABI we weren't built for.
    if (hostVersion != ODBG_PLUGIN_ABI_VERSION) return 0;

    // Document our menu items in the Command Reference window ("?" on the
    // Command pane), under "Plugin commands". Purely informational.
    Odbg_RegisterCommand("sample: Dump state", "Log registers, PEB and bytes at RIP");
    Odbg_RegisterCommand("sample: Set RAX=0",  "Demonstrate Odbg_Setreg");
    Odbg_RegisterCommand("sample: Step",       "Single-step the target (Odbg_Stepinto)");
    Odbg_RegisterCommand("sample: Go",         "Resume the target (Odbg_Go)");
    Odbg_RegisterCommand("sample: BP ntdll!NtTerminateProcess", "Add a breakpoint (Odbg_Addbreakpoint)");
    Odbg_RegisterCommand("sample: Toggle chatty", "Persisted setting (Odbg_Setsetting/Getsetting)");

    // Load our persisted setting. An unset key reads back as an empty string,
    // which is how we spot a first run and keep our own default.
    char chatty[8] = "";
    Odbg_Getsetting("chatty", chatty, sizeof(chatty));
    if (chatty[0]) g_chatty = chatty[0] != '0';

    Odbg_Log("[sample] initialised - demonstrates the full plugin SDK");
    return 1;
}

extern "C" __declspec(dllexport) int Odbg_Pluginmenu(int /*origin*/, char items[][32], int maxItems) {
    int n = 0;
    if (n < maxItems) strcpy_s(items[n++], 32, "Dump state");
    if (n < maxItems) strcpy_s(items[n++], 32, "Set RAX = 0");
    if (n < maxItems) strcpy_s(items[n++], 32, "Step into");
    if (n < maxItems) strcpy_s(items[n++], 32, "Go");
    if (n < maxItems) strcpy_s(items[n++], 32, "BP NtTerminateProcess");
    if (n < maxItems) strcpy_s(items[n++], 32, "Toggle chatty (persisted)");
    return n;
}

extern "C" __declspec(dllexport) void Odbg_Pluginaction(int /*origin*/, int action) {
    // Guard live-target calls: Odbg_SessionActive / Odbg_Stopped tell you
    // whether a target exists and whether it is currently halted.
    bool active = Odbg_SessionActive();
    bool stopped = Odbg_Stopped();

    switch (action) {
    case kMenuDumpState: {
        if (!active || !stopped) { Odbg_Log("[sample] no stopped target to inspect"); return; }

        // Whole register file in one shot...
        OdbgRegs regs{};
        Odbg_Getregs(&regs);
        char buf[160];
        sprintf_s(buf, "[sample] rip=%llx rsp=%llx rax=%llx rcx=%llx rdx=%llx",
            regs.rip, regs.rsp, regs.rax, regs.rcx, regs.rdx);
        Odbg_Log(buf);

        // ...or one register by name.
        unsigned long long rip = Odbg_Getreg(L"rip");

        // Read memory: first 16 bytes at RIP.
        unsigned char code[16] = {};
        if (Odbg_Readmemory(rip, code, sizeof(code))) {
            char hex[64] = "[sample] bytes @ rip:";
            for (int i = 0; i < 16; i++) { char b[4]; sprintf_s(b, " %02x", code[i]); strcat_s(hex, b); }
            Odbg_Log(hex);
        }

        // The debuggee's PEB - the anti-anti-debug plugin patches fields here.
        unsigned long long peb = Odbg_Getpeb();
        sprintf_s(buf, "[sample] PEB = %llx", peb);
        Odbg_Log(buf);
        break;
    }
    case kMenuSetRax: {
        if (!active || !stopped) { Odbg_Log("[sample] need a stopped target to set a register"); return; }
        // Write a single register, then a byte of memory to show Writememory.
        Odbg_Setreg(L"rax", 0);
        unsigned char nop = 0x90;
        unsigned long long rsp = Odbg_Getreg(L"rsp");
        Odbg_Writememory(rsp, &nop, 1);   // harmless scribble on the stack top
        Odbg_Log("[sample] set RAX=0 and wrote a byte at RSP");
        break;
    }
    case kMenuStep:
        if (!active) { Odbg_Log("[sample] no target"); return; }
        Odbg_Stepinto();   // Odbg_Stepover() is the step-over counterpart
        Odbg_Log("[sample] stepped into");
        break;
    case kMenuGo:
        if (!active) { Odbg_Log("[sample] no target"); return; }
        Odbg_Go();
        Odbg_Log("[sample] resumed");
        break;
    case kMenuBreakMain:
        if (!active) { Odbg_Log("[sample] no target"); return; }
        // Add a deferred breakpoint by module!symbol; keep the id so we can
        // remove it again.
        if (g_lastBpId >= 0) { Odbg_Removebreakpoint(g_lastBpId); g_lastBpId = -1; }
        g_lastBpId = Odbg_Addbreakpoint(L"ntdll!NtTerminateProcess");
        {
            char buf[80];
            sprintf_s(buf, "[sample] breakpoint id=%d on ntdll!NtTerminateProcess", g_lastBpId);
            Odbg_Log(buf);
        }
        break;
    case kMenuToggleChatty:
        // Write the setting through as soon as it changes as well as at close,
        // so it survives the host being killed rather than exiting.
        g_chatty = !g_chatty;
        Odbg_Setsetting("chatty", g_chatty ? "1" : "0");
        Odbg_Log(g_chatty ? "[sample] chatty on (remembered)" : "[sample] chatty off (remembered)");
        break;
    default:
        break;
    }
}

extern "C" __declspec(dllexport) void Odbg_Paused(int reason, const OdbgRegs* regs) {
    // Called on every stop. `reason` is one of ODBG_PAUSE_BREAKPOINT / _STEP /
    // _ATTACH_OR_LAUNCH; `regs` is the current register file. Keep this light -
    // it runs on the engine's own thread inside event handling.
    if (!g_chatty) return;   // the persisted setting in action
    const char* why =
        reason == ODBG_PAUSE_BREAKPOINT ? "breakpoint" :
        reason == ODBG_PAUSE_STEP       ? "step" : "attach/launch";
    char buf[96];
    sprintf_s(buf, "[sample] paused (%s) at rip=%llx", why, regs ? regs->rip : 0);
    Odbg_Log(buf);
}

extern "C" __declspec(dllexport) void Odbg_Pluginclose(void) {
    // Last chance to persist state - the host writes its settings file right
    // after firing this.
    Odbg_Setsetting("chatty", g_chatty ? "1" : "0");
    Odbg_Log("[sample] closing");
}
