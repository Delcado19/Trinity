#include <cassert>
#include <thread>
#include "core/parse_setting.h"
#include "core/state.h"
#include "core/gameversion.h"
#include "mem/hooks.h"
#include "game/world.h"
#include "game/parry_output.h"

// Model only MinHook and scanning failures; exercise the production helpers.
static bool created = false, failEnable = false;
static size_t matches = 1;
static int removals = 0;
MH_STATUS WINAPI MH_CreateHook(LPVOID, LPVOID, LPVOID* original)
{
    if (created) return MH_ERROR_ALREADY_CREATED;
    created = true;
    *original = reinterpret_cast<void*>(0x20000);
    return MH_OK;
}
MH_STATUS WINAPI MH_EnableHook(LPVOID) { return failEnable ? MH_ERROR_MEMORY_PROTECT : MH_OK; }
MH_STATUS WINAPI MH_DisableHook(LPVOID) { return MH_OK; }
MH_STATUS WINAPI MH_RemoveHook(LPVOID) { created = false; ++removals; return MH_OK; }
namespace trinity::mem
{
    const ModuleRegion& GameModule() { static ModuleRegion region{0x10000, 4096}; return region; }
    uintptr_t FindPattern(std::string_view, const ModuleRegion&) { return matches ? 0x10000 : 0; }
    uintptr_t FindPatternAny(const std::string_view*, size_t, const ModuleRegion& region, size_t* which)
    { *which = 0; return FindPattern("", region); }
    size_t CountMatches(std::string_view, const ModuleRegion&, size_t) { return matches; }
}
static void Detour() {}

int main()
{
    using namespace trinity;
    // Low stack output bytes must be accepted without accepting arbitrary
    // low pointers, null, or the exclusive stack upper bound.
    assert(game::detail::IsParryOutputAddress(0x20001, 0x20000, 0x30000));
    assert(!game::detail::IsParryOutputAddress(0x1FFFF, 0x20000, 0x30000));
    assert(!game::detail::IsParryOutputAddress(0x30000, 0x20000, 0x30000));
    assert(!game::detail::IsParryOutputAddress(0, 0, 0x30000));
    assert(game::detail::IsParryOutputAddress(game::kMinPointer, 0x20000, 0x30000));
    bool verdict = false;
    uint8_t originalVerdict = 0xFF;
    assert(game::detail::ReadParryOutput(&verdict, &originalVerdict) && originalVerdict == 0);
    assert(game::detail::WriteParryOutput(&verdict) && verdict);
    assert(!game::detail::ReadParryOutput(nullptr, &originalVerdict));
    assert(!game::detail::WriteParryOutput(nullptr));
    int day = 0, hour = 0;
    assert(game::detail::ShiftClockHours(2, 23, 2, day, hour) && day == 3 && hour == 1);
    assert(game::detail::ShiftClockHours(2, 0, -1, day, hour) && day == 1 && hour == 23);
    assert(game::detail::ShiftClockHours(0, 0, -240, day, hour) && day == 0 && hour == 0);
    assert(game::detail::ShiftClockHours(1, 3, 240, day, hour) && day == 11 && hour == 3);
    assert(!game::detail::ShiftClockHours(INT_MAX, 23, 1, day, hour));
    assert(!game::detail::ShiftClockHours(-1, 0, 1, day, hour));
    assert(!game::detail::ShiftClockHours(0, 24, 1, day, hour));
    for (const char* bad : {"nan", "NaN", "inf", "-inf", "1e999", "1e-999", "", " ", "3oops"})
        assert(ParseFloatSetting(bad, 2.0f) == 2.0f);
    assert(ParseFloatSetting(" 3.5\r\n", 2.0f) == 3.5f);
    assert(ParseFloatSetting("0", 2.0f) == 0.0f);

    GameVersion version{1, 0, 0, 2976, true};
    assert(version.isVerified());
    version.revision = 65535; assert(!version.isVerified());
    version.revision = 2760; assert(!version.isVerified());
    version.revision = 2976; version.major = 2; assert(!version.isVerified());
    version.major = 1; version.known = false; assert(!version.isVerified());

    State state;
    std::thread writer([&] { for (int i = 0; i < 10000; ++i) state.flightSpeed = float(i); state.menuOpen = true; });
    while (!state.menuOpen.load()) assert(std::isfinite(state.flightSpeed.load()));
    writer.join();
    assert(state.flightSpeed.load() == 9999.0f);

    for (bool alternatives : {false, true})
    {
        void (*original)() = nullptr;
        void* target = nullptr;
        auto install = [&] {
            return alternatives
                ? mem::InstallHookAny("test", {"AA", "BB"}, "test", &Detour, &original, &target)
                : mem::InstallHook("test", "AA", "test", &Detour, &original, &target);
        };
        matches = 1; failEnable = true;
        const int before = removals;
        assert(!install());
        assert(!created && !original && !target && removals == before + 1);
        failEnable = false;
        assert(install()); // retry succeeds after rollback
        mem::RemoveHook(&target);
        created = true; // owned by another caller
        const int owned = removals;
        assert(!install());
        assert(created && !original && !target && removals == owned);
        created = false;
        for (size_t count : {size_t(0), size_t(2)})
        {
            matches = count;
            original = &Detour;
            assert(!install());
            assert(!created && !original && !target);
        }
    }
}
