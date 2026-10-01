#include "../gui/framework.h"
#include "i18n.h"
#include "settings.h"

#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "parse_setting.h"

#include "logger.h"
#include "mod.h"
#include "state.h"

namespace trinity
{
    // Set once by ClaimOwnership() in the process that presents the game. See
    // settings.h - only that process may write Trinity.ini.
    static bool g_owner = false;

    // Trinity.ini lives next to Trinity.asi so the whole install stays one
    // folder that can be copied or deleted as a unit. `suffix` appends to the
    // file name for the temp file Save() writes through.
    static bool IniPath(char* buf, size_t cap, const char* suffix = "")
    {
        const DWORD n = GetModuleFileNameA(Mod::Get().Module(), buf, static_cast<DWORD>(cap));
        if (n == 0 || n >= cap)
            return false;

        char* slash = strrchr(buf, '\\');
        if (!slash)
            return false;

        const size_t left = cap - static_cast<size_t>(slash + 1 - buf);
        return snprintf(slash + 1, left, "Trinity.ini%s", suffix) < static_cast<int>(left);
    }

    static float ClampF(float v, float lo, float hi)
    {
        return v < lo ? lo : v > hi ? hi : v;
    }

    static int ClampI(int v, int lo, int hi)
    {
        return v < lo ? lo : v > hi ? hi : v;
    }

    void Settings::Load()
    {
        char path[MAX_PATH];
        if (!IniPath(path, sizeof(path)))
            return;

        FILE* f = fopen(path, "r");
        if (!f)
            return; // first run - nothing saved yet

        // Parse onto a default-constructed State so missing/garbled keys keep
        // their defaults, then apply in one step below.
        State vals;
        char  line[128];
        while (fgets(line, sizeof(line), f))
        {
            char* eq = strchr(line, '=');
            if (!eq)
                continue;
            *eq = 0;
            const char* key = line;
            const char* val = eq + 1;

            if      (!strcmp(key, "openKeyVk"))       vals.openKeyVk      = atoi(val);
            else if (!strcmp(key, "openPadMask"))     vals.openPadMask    = static_cast<unsigned int>(strtoul(val, nullptr, 0));
            else if (!strcmp(key, "flyUpKeyVk"))      vals.flyUpKeyVk     = atoi(val);
            else if (!strcmp(key, "flyDownKeyVk"))    vals.flyDownKeyVk   = atoi(val);
            else if (!strcmp(key, "flyUpPadMask"))    vals.flyUpPadMask   = static_cast<unsigned int>(strtoul(val, nullptr, 0));
            else if (!strcmp(key, "flyDownPadMask"))  vals.flyDownPadMask = static_cast<unsigned int>(strtoul(val, nullptr, 0));
            else if (!strcmp(key, "autoSave"))        vals.autoSave       = atoi(val) != 0;
            else if (!strcmp(key, "godMode"))         vals.godMode        = atoi(val) != 0;
            else if (!strcmp(key, "oneHitKill"))      vals.oneHitKill     = atoi(val) != 0;
            else if (!strcmp(key, "noFallDamage"))    vals.noFallDamage   = atoi(val) != 0;
            else if (!strcmp(key, "easyParry"))       vals.easyParry      = atoi(val) != 0;
            else if (!strcmp(key, "noBounty"))        vals.noBounty       = atoi(val) != 0;
            else if (!strcmp(key, "language"))        snprintf(vals.language, sizeof(vals.language), "%s", val);
            else if (!strcmp(key, "themeIndex"))      vals.themeIndex     = atoi(val);
            else if (!strcmp(key, "fileLogging"))     vals.fileLogging    = atoi(val) != 0;
            else if (!strcmp(key, "infStamina"))      vals.infStamina     = atoi(val) != 0;
            else if (!strcmp(key, "infSpirit"))       vals.infSpirit      = atoi(val) != 0;
            else if (!strcmp(key, "infMountStamina"))  vals.infMountStamina = atoi(val) != 0;
            else if (!strcmp(key, "immuneFire"))       vals.immuneFire     = atoi(val) != 0;
            else if (!strcmp(key, "immuneCold"))       vals.immuneCold     = atoi(val) != 0;
            else if (!strcmp(key, "dmgOutMult"))      vals.dmgOutMult     = ParseFloatSetting(val, vals.dmgOutMult);
            else if (!strcmp(key, "dmgInMult"))       vals.dmgInMult      = ParseFloatSetting(val, vals.dmgInMult);
            else if (!strcmp(key, "gameSpeed"))       vals.gameSpeed      = atoi(val) != 0;
            else if (!strcmp(key, "gameSpeedMult"))   vals.gameSpeedMult  = ParseFloatSetting(val, vals.gameSpeedMult);
            else if (!strcmp(key, "timeFrozen"))      vals.timeFrozen     = atoi(val) != 0;
            else if (!strcmp(key, "superRun"))        vals.superRun       = atoi(val) != 0;
            else if (!strcmp(key, "superRunMult"))    vals.superRunMult   = ParseFloatSetting(val, vals.superRunMult);
            else if (!strcmp(key, "superJump"))       vals.superJump      = atoi(val) != 0;
            else if (!strcmp(key, "superJumpMult"))   vals.superJumpMult  = ParseFloatSetting(val, vals.superJumpMult);
            else if (!strcmp(key, "freeFlight"))      vals.freeFlight     = atoi(val) != 0;
            else if (!strcmp(key, "flightSpeed"))     vals.flightSpeed    = ParseFloatSetting(val, vals.flightSpeed);
            else if (!strcmp(key, "trustMult"))       vals.trustMult      = atoi(val) != 0;
            else if (!strcmp(key, "trustMultVal"))    vals.trustMultVal   = ParseFloatSetting(val, vals.trustMultVal);
            else if (!strcmp(key, "invSlotSize"))     vals.invSlotSize    = atoi(val) != 0;
            else if (!strcmp(key, "invSlotSizeVal"))  vals.invSlotSizeVal = atoi(val);
            else if (!strcmp(key, "invStackSize"))    vals.invStackSize   = atoi(val) != 0;
            else if (!strcmp(key, "invStackSizeVal")) vals.invStackSizeVal = atoi(val);
            else if (!strcmp(key, "itemPreview"))     vals.itemPreview    = atoi(val) != 0;
            else if (!strcmp(key, "showFps"))         vals.showFps        = atoi(val) != 0;
        }
        fclose(f);

        State& st  = State::Get();
        st.autoSave = vals.autoSave.load();

        // Every key/pad bind persists regardless of Auto Save - a rebind you
        // can't keep between sessions is a bug, not a "feature value". A garbled
        // key falls back to the default; a 0 pad mask legitimately means "no
        // controller bind", so it is honoured as-is (fly binds use 0 to mean
        // "that direction disabled on the pad").
        if (vals.openKeyVk > 0 && vals.openKeyVk <= 0xFF)
            st.openKeyVk = vals.openKeyVk.load();
        st.openPadMask = vals.openPadMask & 0xFFFF;
        if (vals.flyUpKeyVk >= 0 && vals.flyUpKeyVk <= 0xFF)
            st.flyUpKeyVk = vals.flyUpKeyVk.load();
        if (vals.flyDownKeyVk >= 0 && vals.flyDownKeyVk <= 0xFF)
            st.flyDownKeyVk = vals.flyDownKeyVk.load();
        st.flyUpPadMask   = vals.flyUpPadMask   & 0x3FFFF;
        st.flyDownPadMask = vals.flyDownPadMask & 0x3FFFF;

        if (!st.autoSave)
            return; // remembered the preference, but features start clean

        // Clamp the floats to the same ranges the menu rows enforce, in case
        // the file was hand-edited.
        st.godMode         = vals.godMode.load();
        st.oneHitKill      = vals.oneHitKill.load();
        st.noFallDamage    = vals.noFallDamage.load();
        st.easyParry       = vals.easyParry.load();
        st.noBounty        = vals.noBounty.load();
        snprintf(st.language, sizeof(st.language), "%s", vals.language);
        // Apply before the menu first draws, and before the font atlas is built.
        i18n::SetLanguageByCode(st.language);
        st.themeIndex = vals.themeIndex.load();
        ui::SetTheme(st.themeIndex);
        st.fileLogging = vals.fileLogging.load();
        if (!st.fileLogging) trinity::Logger::DisableFile();
        // The old separate mount toggle is now the same feature. Carry an
        // existing enabled setting forward instead of silently turning it off.
        st.infStamina      = vals.infStamina || vals.infMountStamina;
        st.infSpirit       = vals.infSpirit.load();
        st.infMountStamina = false;
        st.immuneFire      = vals.immuneFire.load();
        st.immuneCold      = vals.immuneCold.load();
        st.dmgOutMult      = ClampF(vals.dmgOutMult, 0.0f, 20.0f);
        st.dmgInMult       = ClampF(vals.dmgInMult, 0.0f, 10.0f);
        st.gameSpeed       = vals.gameSpeed.load();
        st.gameSpeedMult   = ClampF(vals.gameSpeedMult, 0.1f, 1.0f);
        st.timeFrozen      = vals.timeFrozen.load();
        st.superRun        = vals.superRun.load();
        st.superRunMult    = ClampF(vals.superRunMult, 1.0f, 10.0f);
        st.superJump       = vals.superJump.load();
        st.superJumpMult   = ClampF(vals.superJumpMult, 1.0f, 10.0f);
        st.freeFlight      = vals.freeFlight.load();
        st.flightSpeed     = ClampF(vals.flightSpeed, 1.0f, 40.0f);
        st.trustMult       = vals.trustMult.load();
        st.trustMultVal    = ClampF(vals.trustMultVal, 1.0f, 25.0f);
        st.invSlotSize     = vals.invSlotSize.load();
        st.invSlotSizeVal  = ClampI(vals.invSlotSizeVal, 1, 9999);
        st.invStackSize    = vals.invStackSize.load();
        st.invStackSizeVal = ClampI(vals.invStackSizeVal, 1, 999999999);
        st.itemPreview     = vals.itemPreview.load();
        st.showFps         = vals.showFps.load();
        LOG_OK("Trinity.ini loaded - restored feature settings from last session.");
    }

    void Settings::ClaimOwnership()
    {
        g_owner = true;
    }

    void Settings::Save()
    {
        // Never let a process without a menu write its startup snapshot back.
        if (!g_owner)
            return;

        char path[MAX_PATH];
        char tmp[MAX_PATH];
        if (!IniPath(path, sizeof(path)) || !IniPath(tmp, sizeof(tmp), ".tmp"))
            return;

        // Write through a temp file and swap it in, so an interrupted save (the
        // shutdown one runs while the process is already tearing down) can never
        // leave a truncated Trinity.ini behind - the old file survives instead.
        const State& st = State::Get();
        FILE* f = fopen(tmp, "w");
        if (!f)
        {
            LOG_WARN("Could not write %s - feature settings not saved.", tmp);
            return;
        }

        fprintf(f,
                "; Trinity feature settings - managed from the in-game SYSTEM tab.\n"
                "; *KeyVk = Win32 virtual-key code; *PadMask = XInput button mask.\n"
                "openKeyVk=%d\n"
                "openPadMask=%u\n"
                "flyUpKeyVk=%d\n"
                "flyDownKeyVk=%d\n"
                "flyUpPadMask=%u\n"
                "flyDownPadMask=%u\n"
                "autoSave=%d\n"
                "godMode=%d\n"
                "oneHitKill=%d\n"
                "noFallDamage=%d\n"
                "easyParry=%d\n"
                "noBounty=%d\n"
                "language=%s\n"
                "themeIndex=%d\n"
                "fileLogging=%d\n"
                "infStamina=%d\n"
                "infSpirit=%d\n"
                "infMountStamina=%d\n"
                "immuneFire=%d\n"
                "immuneCold=%d\n"
                "dmgOutMult=%.3f\n"
                "dmgInMult=%.3f\n"
                "gameSpeed=%d\n"
                "gameSpeedMult=%.3f\n"
                "timeFrozen=%d\n"
                "superRun=%d\n"
                "superRunMult=%.3f\n"
                "superJump=%d\n"
                "superJumpMult=%.3f\n"
                "freeFlight=%d\n"
                "flightSpeed=%.3f\n"
                "trustMult=%d\n"
                "trustMultVal=%.3f\n"
                "invSlotSize=%d\n"
                "invSlotSizeVal=%d\n"
                "invStackSize=%d\n"
                "invStackSizeVal=%d\n"
                "itemPreview=%d\n"
                "showFps=%d\n",
                st.openKeyVk.load(),
                st.openPadMask.load(),
                st.flyUpKeyVk.load(),
                st.flyDownKeyVk.load(),
                st.flyUpPadMask.load(),
                st.flyDownPadMask.load(),
                st.autoSave ? 1 : 0,
                st.godMode ? 1 : 0,
                st.oneHitKill ? 1 : 0,
                st.noFallDamage ? 1 : 0,
                st.easyParry ? 1 : 0,
                st.noBounty ? 1 : 0,
                st.language,
                st.themeIndex.load(),
                st.fileLogging ? 1 : 0,
                st.infStamina ? 1 : 0,
                st.infSpirit ? 1 : 0,
                st.infMountStamina ? 1 : 0,
                st.immuneFire ? 1 : 0,
                st.immuneCold ? 1 : 0,
                st.dmgOutMult.load(),
                st.dmgInMult.load(),
                st.gameSpeed ? 1 : 0,
                st.gameSpeedMult.load(),
                st.timeFrozen ? 1 : 0,
                st.superRun ? 1 : 0,
                st.superRunMult.load(),
                st.superJump ? 1 : 0,
                st.superJumpMult.load(),
                st.freeFlight ? 1 : 0,
                st.flightSpeed.load(),
                st.trustMult ? 1 : 0,
                st.trustMultVal.load(),
                st.invSlotSize ? 1 : 0,
                st.invSlotSizeVal.load(),
                st.invStackSize ? 1 : 0,
                st.invStackSizeVal.load(),
                st.itemPreview ? 1 : 0,
                st.showFps ? 1 : 0);
        const bool ok = fflush(f) == 0;
        fclose(f);

        if (!ok || !MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING))
        {
            LOG_WARN("Could not update %s - feature settings not saved.", path);
            DeleteFileA(tmp);
        }
    }

    void Settings::ResetFeatures()
    {
        // Copy the defaults straight off a fresh State so this can never
        // drift from the initializers in state.h. Menu/session state
        // (menuOpen, textCapture, autoSave) is deliberately left alone.
        const State def;
        State&      st = State::Get();
        st.godMode         = def.godMode.load();
        st.oneHitKill      = def.oneHitKill.load();
        st.noFallDamage    = def.noFallDamage.load();
        st.easyParry       = def.easyParry.load();
        st.noBounty        = def.noBounty.load();
        snprintf(st.language, sizeof(st.language), "%s", def.language);
        st.themeIndex = def.themeIndex.load();
        ui::SetTheme(st.themeIndex);
        st.fileLogging = def.fileLogging.load();
        st.infStamina      = def.infStamina.load();
        st.infSpirit       = def.infSpirit.load();
        st.infMountStamina = def.infMountStamina.load();
        st.immuneFire      = def.immuneFire.load();
        st.immuneCold      = def.immuneCold.load();
        st.dmgOutMult      = def.dmgOutMult.load();
        st.dmgInMult       = def.dmgInMult.load();
        st.gameSpeed       = def.gameSpeed.load();
        st.gameSpeedMult   = def.gameSpeedMult.load();
        st.timeFrozen      = def.timeFrozen.load();
        st.superRun        = def.superRun.load();
        st.superRunMult    = def.superRunMult.load();
        st.superJump       = def.superJump.load();
        st.superJumpMult   = def.superJumpMult.load();
        st.freeFlight      = def.freeFlight.load();
        st.flightSpeed     = def.flightSpeed.load();
        st.trustMult       = def.trustMult.load();
        st.trustMultVal    = def.trustMultVal.load();
        st.invSlotSize     = def.invSlotSize.load();
        st.invSlotSizeVal  = def.invSlotSizeVal.load();
        st.invStackSize    = def.invStackSize.load();
        st.invStackSizeVal = def.invStackSizeVal.load();
        st.itemPreview     = def.itemPreview.load();
        st.showFps         = def.showFps.load();
    }

    void Settings::ResetBinds()
    {
        const State def;
        State&      st = State::Get();
        st.openKeyVk      = def.openKeyVk.load();
        st.openPadMask    = def.openPadMask.load();
        st.flyUpKeyVk     = def.flyUpKeyVk.load();
        st.flyDownKeyVk   = def.flyDownKeyVk.load();
        st.flyUpPadMask   = def.flyUpPadMask.load();
        st.flyDownPadMask = def.flyDownPadMask.load();
    }
}
