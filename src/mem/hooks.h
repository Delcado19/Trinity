#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <initializer_list>

#include <MinHook.h>

#include "scanner.h"
#include "../core/logger.h"

namespace trinity::mem
{
    // Health of a feature's backing signature: NOT FOUND (Dead), found but only
    // via an older-build fallback pattern (Fallback), or the current build's own
    // pattern matched outright (Ok). Independent of whether a game world is
    // currently loaded - that is Ready()/Available(), a separate concept - so the
    // menu's status dot (gui/widgets.h) can tell "this build's signature drifted"
    // apart from "no player object right now".
    enum class SigStatus { Dead, Fallback, Ok };
    inline SigStatus StatusOf(bool installed, size_t which)
    {
        return !installed ? SigStatus::Dead : which > 0 ? SigStatus::Fallback : SigStatus::Ok;
    }

    // Combines two statuses for a feature that needs both signatures healthy -
    // the worse of the two wins (Dead < Fallback < Ok by declaration order).
    inline SigStatus Worse(SigStatus a, SigStatus b)
    {
        return static_cast<int>(a) < static_cast<int>(b) ? a : b;
    }

    // Finds `sig` (refusing an ambiguous match, up to `maxMatches` probes) and
    // installs a MinHook detour at the resolved address - the
    // find/warn/hook/log sequence every game/*.cpp Install() otherwise
    // repeats by hand. `context` prefixes every log line (e.g.
    // "player: stat-accessor") and `consequence` names what's lost on
    // failure (e.g. "God Mode disabled"), so a call site supplies two
    // feature-specific strings instead of three near-duplicate LOG_* calls.
    //
    // On success, *original is set to the trampoline and *target to the
    // hooked address (truthy - keep it to pass to RemoveHook in Remove());
    // both are left null on failure. Returns whether the hook is installed -
    // callers decide whether a false return is fatal for the whole feature
    // or just one sub-feature.
    template <typename Fn>
    bool InstallHook(const char* context, std::string_view sig, const char* consequence,
                      Fn detour, Fn* original, void** target, size_t maxMatches = 8)
    {
        *target = nullptr;
        *original = nullptr;
        const uintptr_t addr = FindPattern(sig);
        if (!addr)
        {
            LOG_ERR("%s signature NOT FOUND - %s.", context, consequence);
            return false;
        }

        const size_t matches = CountMatches(sig, maxMatches);
        if (matches != 1)
        {
            LOG_ERR("%s signature ambiguous (%zu); refusing hook - %s.",
                    context, matches, consequence);
            return false;
        }

        void* t = reinterpret_cast<void*>(addr);
        const MH_STATUS created = MH_CreateHook(t, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original));
        if (created != MH_OK || MH_EnableHook(t) != MH_OK)
        {
            // Only remove our own newly created hook; otherwise retries can
            // stay stuck on MH_ERROR_ALREADY_CREATED after an enable failure.
            if (created == MH_OK) MH_RemoveHook(t);
            LOG_ERR("%s: failed to install hook - %s.", context, consequence);
            *original = nullptr;
            return false;
        }

        *target = t;
        return true;
    }

    // InstallHook over a primary + fallback pattern list. Index 0 is the
    // current build's pattern; anything after it is a previous build's, kept so
    // one drifted signature degrades to "matched the older shape" instead of a
    // dead feature. Logs which variant matched, because that is exactly the
    // detail a post-patch bug report needs.
    // `outWhich`, when given, receives the matched pattern's index on success
    // (0 = current build's own pattern, >0 = an older build's fallback) - feeds
    // StatusOf() for the menu's status dot. Left untouched on failure; callers
    // that only care about the hook being installed just omit it.
    template <typename Fn>
    bool InstallHookAny(const char* context, std::initializer_list<std::string_view> sigs,
                        const char* consequence, Fn detour, Fn* original, void** target,
                        size_t* outWhich = nullptr)
    {
        *target = nullptr;
        *original = nullptr;
        size_t which = 0;
        const uintptr_t addr = FindPatternAny(sigs, &which);
        if (!addr)
        {
            LOG_ERR("%s signature NOT FOUND (tried %zu variants) - %s.",
                    context, sigs.size(), consequence);
            return false;
        }
        if (which > 0)
            LOG_WARN("%s matched fallback pattern #%zu - this game build moved the "
                     "current one, so please report it.", context, which);

        const size_t matches = CountMatches(*(sigs.begin() + which));
        if (matches != 1)
        {
            LOG_ERR("%s signature ambiguous (%zu); refusing hook - %s.",
                    context, matches, consequence);
            return false;
        }

        void* t = reinterpret_cast<void*>(addr);
        const MH_STATUS created = MH_CreateHook(t, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original));
        if (created != MH_OK || MH_EnableHook(t) != MH_OK)
        {
            // Never remove a hook owned by another caller on create failure.
            if (created == MH_OK) MH_RemoveHook(t);
            LOG_ERR("%s: failed to install hook - %s.", context, consequence);
            *original = nullptr;
            return false;
        }
        *target = t;
        if (outWhich) *outWhich = which;
        return true;
    }

    // Disables + removes a hook installed via InstallHook and clears
    // *target, so Remove() can call this unconditionally and stays
    // idempotent (a null target is a no-op).
    inline void RemoveHook(void** target)
    {
        if (!*target) return;
        MH_DisableHook(*target);
        MH_RemoveHook(*target);
        *target = nullptr;
    }
}
