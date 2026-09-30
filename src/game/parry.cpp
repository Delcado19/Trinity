#include "parry.h"

#include <Windows.h>
#include <cstring>

#include "offsets.h"
#include "../core/logger.h"
#include "../hooks/xinput_hook.h"
#include "../mem/hooks.h"
#include "../mem/safe_memory.h"
#include "../mem/scanner.h"

namespace trinity::game
{
    namespace
    {
        // vcomiss xmm2, xmm3 ; seta al ; mov byte ptr [rsi], al
        // Unique in the image: the comparison, the flag it produces and the
        // store of that flag are all in the pattern, so it identifies the parry
        // verdict itself rather than a shape that happens to recur.
        constexpr const char* kSig_ParryVerdict = "C5 F8 2F D3 0F 97 C0 88 06";

        // TU 2.01.00 candidate (COMPATIBILITY.md: "BREAKTHROUGH 2026-09-08").
        // The compiler swapped the VCOMISS operand order on this build
        // (`VCOMISS XMM2,XMM3` -> `VCOMISS XMM3,XMM2`), changing only the
        // ModRM byte (D3 -> DA); the SETA + store that follow (the actual
        // patch site, kSetaOffset below) are byte-identical. Confirmed via a
        // working third-party build's live-logged address (RVA 0x7FC528,
        // `parry: verdict site @ ...`): Ghidra shows SETA AL starting at
        // exactly that RVA, 4 bytes after this pattern's start. Verified
        // offline: one match, exactly RVA 0x7FC524 (kSetaOffset 4 lands on
        // 0x7FC528). NOT yet live-tested through our own patch code.
        constexpr const char* kSig_ParryVerdict_TU20100_Candidate = "C5 F8 2F DA 0F 97 C0 88 06";

        constexpr uintptr_t kSetaOffset = 4;         // into the match
        constexpr uint8_t   kSeta[3] = { 0x0F, 0x97, 0xC0 };  // seta al
        constexpr uint8_t   kForce[3] = { 0xB0, 0x01, 0x90 }; // mov al,1 ; nop

        uintptr_t g_site = 0;    // address of the seta (patch fallback only)
        bool      g_on   = false;

        // --- Evaluator hook (preferred) --------------------------------------
        // The verdict patch above forces the attacker-side timing verdict, but
        // that alone does NOT make a HELD block parry: the player's guard state
        // machine reacts to the press edge, so a block that is held down never
        // registers however the verdict is forced (measured by gugi97's fork,
        // upstream e0d287e, and the reason "Easy Parry on" did nothing for a
        // held LB/LT on 2976, live 2026-09-30). The fix is to hook the whole
        // evaluator (RVA 0x8738B0 on 2976, the same function the vTweak build
        // hooks) and, when the parry window opens, release and re-press
        // whatever is held so the game sees a fresh press edge. Pattern and
        // approach from gugi97's fork. The verdict patch stays as a fallback
        // for builds where the evaluator pattern no longer matches.
        constexpr const char* kSig_ParryEvaluator =
            "48 8B C4 41 55 41 56 41 57 48 83 EC 70 C5 78 29 40 A8 "
            "48 89 58 08 4C 8D 2D ?? ?? ?? ?? 48 89 68 10 41 B8 02 00 00 00 "
            "44 39 05 ?? ?? ?? ?? 4C 8B FA 48 89 70 18 4C 8B F1";

        using ParryEvaluatorFn = bool(__fastcall*)(void*, void*, float, bool, bool*);
        ParryEvaluatorFn g_original = nullptr;
        void*            g_target   = nullptr;
        ULONGLONG        g_lastPulse = 0;

        // A rate, not an edge: consecutive swings keep the window open, so a
        // rising edge only parried the first attack of a combo.
        constexpr ULONGLONG kPulseIntervalMs = 250;

        // Keyboard and mouse never reach the pad layer (raw input delivers
        // events, not state), so a held key is released and pressed again with
        // SendInput. Movement keys are deliberately absent.
        constexpr int kPulseKeys[] = {
            VK_RBUTTON, VK_LBUTTON, VK_MBUTTON,
            VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_SPACE,
            'Q', 'E', 'R', 'F', 'C', 'V',
        };

        void PulseHeldKeys()
        {
            INPUT in[2 * (sizeof(kPulseKeys) / sizeof(kPulseKeys[0]))] = {};
            int n = 0;
            for (const int vk : kPulseKeys)
            {
                if (!(GetAsyncKeyState(vk) & 0x8000))
                    continue;
                DWORD down = 0, up = 0;
                if      (vk == VK_RBUTTON) { down = MOUSEEVENTF_RIGHTDOWN;  up = MOUSEEVENTF_RIGHTUP; }
                else if (vk == VK_LBUTTON) { down = MOUSEEVENTF_LEFTDOWN;   up = MOUSEEVENTF_LEFTUP; }
                else if (vk == VK_MBUTTON) { down = MOUSEEVENTF_MIDDLEDOWN; up = MOUSEEVENTF_MIDDLEUP; }
                if (down)
                {
                    in[n].type = INPUT_MOUSE;    in[n].mi.dwFlags = up;   ++n;
                    in[n].type = INPUT_MOUSE;    in[n].mi.dwFlags = down; ++n;
                    continue;
                }
                const WORD scan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
                for (int pass = 0; pass < 2; ++pass)
                {
                    in[n].type       = INPUT_KEYBOARD;
                    in[n].ki.wVk     = static_cast<WORD>(vk);
                    in[n].ki.wScan   = scan;
                    in[n].ki.dwFlags = pass == 0 ? KEYEVENTF_KEYUP : 0;
                    ++n;
                }
            }
            if (n > 0)
                SendInput(static_cast<UINT>(n), in, sizeof(INPUT));
        }

        bool __fastcall hkParryEvaluator(void* a, void* b, float range, bool evade, bool* perfect)
        {
            const bool eligible = g_original ? g_original(a, b, range, evade, perfect) : false;
            if (evade || !g_on)
                return eligible;

            const ULONGLONG now = GetTickCount64();
            if (eligible && now - g_lastPulse >= kPulseIntervalMs)
            {
                hooks::PulseButtonRelease();   // pad
                PulseHeldKeys();               // keyboard and mouse
                g_lastPulse = now;
            }
            if (eligible && perfect)
                mem::Write8(reinterpret_cast<uintptr_t>(perfect), 1);
            return eligible;
        }

        // Write three bytes over executable code, restoring protection either
        // way. Failure leaves the site untouched rather than half-written.
        bool WriteCode(uintptr_t addr, const uint8_t (&bytes)[3])
        {
            DWORD old = 0;
            if (!VirtualProtect(reinterpret_cast<void*>(addr), sizeof(bytes),
                                PAGE_EXECUTE_READWRITE, &old))
                return false;
            std::memcpy(reinterpret_cast<void*>(addr), bytes, sizeof(bytes));
            DWORD ignored = 0;
            VirtualProtect(reinterpret_cast<void*>(addr), sizeof(bytes), old, &ignored);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(addr), sizeof(bytes));
            return true;
        }
    }

    bool Parry::Install()
    {
        // Preferred path: the evaluator hook (handles a held block). Falls
        // through to the verdict patch below when the pattern does not match.
        if (mem::InstallHook("parry: evaluator", kSig_ParryEvaluator,
                             "falling back to the verdict patch",
                             &hkParryEvaluator, &g_original, &g_target))
        {
            LOG("parry: evaluator hook installed @ %p - Easy Parry available.", g_target);
            return true;
        }

        // TU 2.01.00 candidate (COMPATIBILITY.md: "BREAKTHROUGH 2026-09-08")
        // as a fallback - that build's compiler swapped the VCOMISS operand
        // order (only the ModRM byte differs), so the SETA+store this
        // patches is unchanged at kSetaOffset either way.
        size_t which = 0;
        const std::string_view sigs[] = { kSig_ParryVerdict, kSig_ParryVerdict_TU20100_Candidate };
        const uintptr_t hit = mem::FindPatternAny(sigs, 2, mem::GameModule(), &which);
        if (!hit)
        {
            LOG("parry: verdict site not found - Easy Parry disabled.");
            return false;
        }
        if (mem::CountMatches(sigs[which], 4) != 1)
        {
            // Refuse rather than pick one. This patches executable code, and a
            // second match would mean the pattern no longer identifies the
            // thing it was derived from.
            LOG_WARN("parry: verdict site is ambiguous - Easy Parry disabled rather than "
                     "patch a guess.");
            return false;
        }

        g_site = hit + kSetaOffset;
        // Confirm the exact instruction before ever writing over it.
        if (std::memcmp(reinterpret_cast<void*>(g_site), kSeta, sizeof(kSeta)) != 0)
        {
            LOG_WARN("parry: verdict site does not start with the expected instruction - "
                     "Easy Parry disabled.");
            g_site = 0;
            return false;
        }
        LOG("parry: verdict site @ %p - Easy Parry available.", reinterpret_cast<void*>(g_site));
        return true;
    }

    void Parry::Remove()
    {
        if (g_site && g_on)
            WriteCode(g_site, kSeta);   // never leave the game's code modified
        mem::RemoveHook(&g_target);
        g_original = nullptr;
        g_site = 0;
        g_on = false;
    }

    bool Parry::Available() { return g_target != nullptr || g_site != 0; }
    bool Parry::Enabled()   { return g_on; }

    void Parry::SetEnabled(bool on)
    {
        if (on == g_on) return;
        if (g_target)   // hook mode: the hook itself checks g_on
        {
            g_on = on;
            LOG("parry: Easy Parry %s.", on ? "on" : "off");
            return;
        }
        if (!g_site) return;
        if (WriteCode(g_site, on ? kForce : kSeta))
        {
            g_on = on;
            LOG("parry: Easy Parry %s.", on ? "on" : "off");
        }
        else
        {
            LOG_WARN("parry: could not write the verdict site - Easy Parry unchanged.");
        }
    }
}
