# Source and Build Baseline

## Imported source

- Repository: `https://github.com/gugi97/Trinity`
- Branch: `fork/tu2.00.00`
- Commit: `7ad5fb1895021d7132083b4194ca1b1ab208610b`
- Local immutable tag: `baseline/tu2.00.00-gugi-7ad5fb1`
- Port branch: `port/tu-2.01.00`
- Source version: Trinity `0.18.0`
- Recognized baseline game build: Crimson Desert `2.00.00`, PE revision `2625`

The tag points at the unmodified imported commit. Auditor and port work starts
after that tag; no TU 2.01.00 offsets or signatures are inferred by the import.

## Unmodified Release build

- Result: PASS on 2026-09-05
- Generator: Visual Studio 18 2026, x64
- Configuration: Release
- CMake: `4.3.1-msvc1`
- Compiler: MSVC `19.51.36256.0` (`14.51.36231` tools)
- Windows SDK: `10.0.26100.0`
- Output: `build/Release/Trinity.asi`
- Output SHA-256: `96d7ecf00b7cdd04793ec68ab4d5f75aec3c0540dc742c179f80ceabc7747dc9`

Resolved FetchContent dependencies for this build:

| Dependency | Declared revision | Resolved commit |
| --- | --- | --- |
| Dear ImGui | `v1.91.5-docking` | `368123ab06b2b573d585e52f84cd782c5c006697` |
| MinHook | `master` | `d94c64d32ea37bc4f5ee47d580709f70c6fb6080` |

MinHook is not pinned by the imported source. Pinning the recorded commit should
be a separate reproducibility change, not folded into the untouched baseline.

The normal inherited agent environment contained both `PATH` and `Path`, which
caused MSBuild `MSB6001`. Running CMake/MSBuild in a clean process environment
removed that harness-only conflict; no source change was required.
