#pragma once
#include <Windows.h>

namespace trinity
{
    // Top-level coordinator: brings up MinHook + the render/input hooks, and
    // tears them back down on unload.
    class Mod
    {
    public:
        static Mod& Get()
        {
            static Mod instance;
            return instance;
        }

        void Initialize(HMODULE module);
        void Shutdown();

        HMODULE Module() const { return m_module; }

    private:
        Mod() = default;

        HMODULE m_module = nullptr;
        bool    m_initialized = false;
        bool    m_gameplayHooksInstalled = false;
        bool    m_accessorProbeInstalled = false;
        bool    m_damageProbeInstalled = false;
        bool    m_playerStatsInstalled = false;
        bool    m_worldInstalled = false;
        bool    m_parryInstalled = false;
        bool    m_teleportInstalled = false;
    };
}
