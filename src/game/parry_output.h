#pragma once
#include <Windows.h>
#include <cstdint>
#include "offsets.h"

namespace trinity::game::detail
{
    // Engine output parameters may point into the calling thread's stack,
    // below the heap-oriented kMinPointer guard (runtime review 2026-10-03).
    // Do not lower the shared pointer-chain guard to accommodate them.
    constexpr bool IsParryOutputAddress(uintptr_t address, uintptr_t stackLow,
                                        uintptr_t stackHigh)
    {
        return address >= kMinPointer ||
               (address >= 0x10000 && address >= stackLow && address < stackHigh);
    }

    inline bool ReadParryOutput(bool* output, uint8_t* value)
    {
        const auto* tib = reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
        if (!IsParryOutputAddress(reinterpret_cast<uintptr_t>(output),
                reinterpret_cast<uintptr_t>(tib->StackLimit),
                reinterpret_cast<uintptr_t>(tib->StackBase))) return false;
        __try { *value = *reinterpret_cast<volatile uint8_t*>(output); return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    inline bool WriteParryOutput(bool* output)
    {
        const auto* tib = reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
        if (!IsParryOutputAddress(reinterpret_cast<uintptr_t>(output),
                reinterpret_cast<uintptr_t>(tib->StackLimit),
                reinterpret_cast<uintptr_t>(tib->StackBase))) return false;
        __try { *reinterpret_cast<volatile uint8_t*>(output) = 1; return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}
