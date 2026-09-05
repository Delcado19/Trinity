#!/usr/bin/env python3
"""Read-only offline AOB audit for Crimson Desert executables."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import mmap
import os
import struct
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable


IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_MEM_READ = 0x40000000
MAX_RECORDED_LOCATIONS = 64


class AuditError(ValueError):
    """Raised when an input or manifest cannot be audited safely."""


@dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    raw_offset: int
    raw_size: int
    characteristics: int

    @property
    def executable(self) -> bool:
        return bool(self.characteristics & IMAGE_SCN_MEM_EXECUTE)

    @property
    def readable(self) -> bool:
        return bool(self.characteristics & IMAGE_SCN_MEM_READ)


@dataclass(frozen=True)
class Pattern:
    values: tuple[int | None, ...]
    anchor_offset: int
    anchor: bytes


@dataclass(frozen=True)
class PeInfo:
    timestamp: int
    size_of_image: int
    sections: tuple[Section, ...]


def _unpack(fmt: str, data: Any, offset: int) -> tuple[Any, ...]:
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise AuditError("truncated PE structure")
    return struct.unpack_from(fmt, data, offset)


def parse_pe(data: Any) -> PeInfo:
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise AuditError("not a PE file: DOS MZ header missing")
    pe_offset = _unpack("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise AuditError("not a PE file: NT header missing")

    file_header = pe_offset + 4
    _, section_count, timestamp, _, _, optional_size, _ = _unpack(
        "<HHIIIHH", data, file_header
    )
    optional = file_header + 20
    magic = _unpack("<H", data, optional)[0]
    if magic not in (0x10B, 0x20B):
        raise AuditError(f"unsupported PE optional-header magic 0x{magic:X}")
    size_of_image = _unpack("<I", data, optional + 56)[0]

    section_table = optional + optional_size
    sections: list[Section] = []
    for index in range(section_count):
        offset = section_table + index * 40
        raw_name = bytes(data[offset : offset + 8])
        if len(raw_name) != 8:
            raise AuditError("truncated PE section table")
        name = raw_name.split(b"\0", 1)[0].decode("ascii", errors="replace")
        _, virtual_address, raw_size, raw_offset = _unpack("<IIII", data, offset + 8)
        characteristics = _unpack("<I", data, offset + 36)[0]
        sections.append(Section(name, virtual_address, raw_offset, raw_size, characteristics))
    return PeInfo(timestamp, size_of_image, tuple(sections))


def parse_pattern(text: str) -> Pattern:
    values: list[int | None] = []
    for token in text.split():
        if token in ("?", "??"):
            values.append(None)
            continue
        if len(token) != 2:
            raise AuditError(f"invalid pattern token: {token!r}")
        try:
            values.append(int(token, 16))
        except ValueError as exc:
            raise AuditError(f"invalid pattern token: {token!r}") from exc
    if not values:
        raise AuditError("empty pattern")

    best_start = best_length = 0
    run_start = run_length = 0
    for index, value in enumerate((*values, None)):
        if value is not None:
            if run_length == 0:
                run_start = index
            run_length += 1
        else:
            if run_length > best_length:
                best_start, best_length = run_start, run_length
            run_length = 0
    if best_length == 0:
        raise AuditError("all-wildcard patterns are not auditable")
    anchor = bytes(value for value in values[best_start : best_start + best_length] if value is not None)
    return Pattern(tuple(values), best_start, anchor)


def _matches_at(data: Any, offset: int, pattern: Pattern) -> bool:
    return all(value is None or data[offset + index] == value for index, value in enumerate(pattern.values))


def scan_sections(data: Any, sections: Iterable[Section], pattern: Pattern) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    pattern_size = len(pattern.values)
    for section in sections:
        start = section.raw_offset
        end = min(len(data), start + section.raw_size)
        if start < 0 or end - start < pattern_size:
            continue
        search_at = start + pattern.anchor_offset
        anchor_end = end - pattern_size + pattern.anchor_offset + len(pattern.anchor)
        while True:
            anchor_at = data.find(pattern.anchor, search_at, anchor_end)
            if anchor_at < 0:
                break
            candidate = anchor_at - pattern.anchor_offset
            if _matches_at(data, candidate, pattern):
                results.append(
                    {
                        "rva": candidate - section.raw_offset + section.virtual_address,
                        "section": section.name,
                    }
                )
            search_at = anchor_at + 1
    return results


def classify_count(policy: str, expected: int | None, count: int) -> str:
    if policy == "unique":
        return "MISSING" if count == 0 else "EXPECTED_COUNT" if count == 1 else "AMBIGUOUS"
    if policy == "exact":
        return "EXPECTED_COUNT" if count == expected else "MISSING" if count == 0 else "COUNT_MISMATCH"
    if policy == "contextual":
        return "MISSING" if count == 0 else "CONTEXT_REQUIRED"
    if policy == "alternative":
        return "NOT_FOUND" if count == 0 else "ALTERNATIVE_PRESENT" if count == 1 else "AMBIGUOUS"
    if policy == "observe":
        return "NOT_FOUND" if count == 0 else "OBSERVED"
    raise AuditError(f"unknown count policy: {policy!r}")


def resolve_rip(data: Any, section_by_name: dict[str, Section], hit: dict[str, Any], instruction_offset: int) -> int | None:
    section = section_by_name[hit["section"]]
    instruction_rva = hit["rva"] + instruction_offset
    instruction_raw = section.raw_offset + instruction_rva - section.virtual_address
    if instruction_raw < 0 or instruction_raw + 7 > len(data):
        return None
    displacement = _unpack("<i", data, instruction_raw + 3)[0]
    return instruction_rva + 7 + displacement


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _file_version(path: Path) -> str | None:
    if os.name != "nt":
        return None

    class FixedFileInfo(ctypes.Structure):
        _fields_ = [
            ("signature", ctypes.c_uint32),
            ("struct_version", ctypes.c_uint32),
            ("file_version_ms", ctypes.c_uint32),
            ("file_version_ls", ctypes.c_uint32),
            ("product_version_ms", ctypes.c_uint32),
            ("product_version_ls", ctypes.c_uint32),
            ("file_flags_mask", ctypes.c_uint32),
            ("file_flags", ctypes.c_uint32),
            ("file_os", ctypes.c_uint32),
            ("file_type", ctypes.c_uint32),
            ("file_subtype", ctypes.c_uint32),
            ("file_date_ms", ctypes.c_uint32),
            ("file_date_ls", ctypes.c_uint32),
        ]

    version = ctypes.WinDLL("version", use_last_error=True)
    size = version.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return None
    buffer = ctypes.create_string_buffer(size)
    if not version.GetFileVersionInfoW(str(path), 0, size, buffer):
        return None
    pointer = ctypes.c_void_p()
    length = ctypes.c_uint()
    if not version.VerQueryValueW(buffer, "\\", ctypes.byref(pointer), ctypes.byref(length)):
        return None
    info = ctypes.cast(pointer, ctypes.POINTER(FixedFileInfo)).contents
    return ".".join(
        str(part)
        for part in (
            info.file_version_ms >> 16,
            info.file_version_ms & 0xFFFF,
            info.file_version_ls >> 16,
            info.file_version_ls & 0xFFFF,
        )
    )


def audit(executable: Path, manifest: dict[str, Any], game_version: str) -> dict[str, Any]:
    with executable.open("rb") as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as image:
        pe = parse_pe(image)
        primary = tuple(
            section
            for section in pe.sections
            if not section.name.lower().startswith(".debug")
            and (section.executable or section.name.lower() == ".link")
        )
        fallback = tuple(
            section
            for section in pe.sections
            if not section.name.lower().startswith(".debug")
            and section.readable
            and section not in primary
        )
        section_by_name = {section.name: section for section in pe.sections}
        results: dict[str, Any] = {}
        for entry in manifest["signatures"]:
            parsed = parse_pattern(entry["pattern"])
            hits = scan_sections(image, primary, parsed)
            scope = "executable-or-link"
            if not hits and entry.get("readableFallback", True):
                hits = scan_sections(image, fallback, parsed)
                if hits:
                    scope = "readable-fallback"
            resolved: list[str] = []
            if "ripInstructionOffset" in entry:
                for hit in hits:
                    target = resolve_rip(image, section_by_name, hit, entry["ripInstructionOffset"])
                    if target is not None:
                        resolved.append(f"0x{target:X}")
            policy = entry.get("policy", "unique")
            result = {
                "feature": entry["feature"],
                "source": entry["source"],
                "policy": policy,
                "expectedMatches": entry.get("expectedMatches"),
                "matches": len(hits),
                "locationsRecorded": min(len(hits), MAX_RECORDED_LOCATIONS),
                "locationsTruncated": len(hits) > MAX_RECORDED_LOCATIONS,
                "scanScope": scope if hits else "none",
                "countStatus": classify_count(policy, entry.get("expectedMatches"), len(hits)),
                "semanticStatus": "UNKNOWN",
                "locations": [
                    {"rva": f"0x{hit['rva']:X}", "section": hit["section"]}
                    for hit in hits[:MAX_RECORDED_LOCATIONS]
                ],
            }
            if resolved:
                result["resolvedRipRvas"] = resolved
            if entry.get("aliases"):
                result["aliases"] = entry["aliases"]
            if entry.get("alternativeGroup"):
                result["alternativeGroup"] = entry["alternativeGroup"]
            results[entry["id"]] = result

    timestamp_utc = datetime.fromtimestamp(pe.timestamp, timezone.utc).isoformat().replace("+00:00", "Z")
    return {
        "schemaVersion": 1,
        "game": "Crimson Desert",
        "gameVersion": game_version,
        "baseline": manifest["baseline"],
        "executable": {
            "name": executable.name,
            "size": executable.stat().st_size,
            "sha256": _sha256(executable),
            "fileVersion": _file_version(executable),
            "peTimestamp": f"0x{pe.timestamp:08X}",
            "peTimestampUtc": timestamp_utc,
            "sizeOfImage": f"0x{pe.size_of_image:X}",
        },
        "scan": {
            "primarySections": [section.name for section in primary],
            "fallbackSections": [section.name for section in fallback],
            "excludedSectionPrefix": ".debug",
            "maxRecordedLocationsPerSignature": MAX_RECORDED_LOCATIONS,
            "note": "Count results are mechanical only; semantic status remains UNKNOWN.",
        },
        "signatures": results,
    }


def markdown_report(report: dict[str, Any]) -> str:
    exe = report["executable"]
    lines = [
        "# Crimson Desert Trinity Signature Audit",
        "",
        f"- Game version: `{report['gameVersion']}`",
        f"- Baseline: `{report['baseline']['label']}`",
        f"- Baseline commit: `{report['baseline']['commit']}`",
        f"- Executable: `{exe['name']}`",
        f"- File version: `{exe['fileVersion'] or 'unavailable'}`",
        f"- SHA-256: `{exe['sha256']}`",
        f"- PE timestamp: `{exe['peTimestamp']}` ({exe['peTimestampUtc']})",
        f"- SizeOfImage: `{exe['sizeOfImage']}`",
        f"- Primary PE sections: `{', '.join(report['scan']['primarySections'])}`",
        "",
        "> A matching byte count does not verify function semantics, calling conventions, or layouts.",
        "",
        "| Signature | Feature | Matches | Count status | Scope | RVAs |",
        "| --- | --- | ---: | --- | --- | --- |",
    ]
    for signature_id, result in report["signatures"].items():
        rvas = ", ".join(location["rva"] for location in result["locations"])
        if len(rvas) > 160:
            rvas = rvas[:157] + "..."
        lines.append(
            f"| `{signature_id}` | {result['feature']} | {result['matches']} | "
            f"{result['countStatus']} | {result['scanScope']} | {rvas or '-'} |"
        )
    lines.extend(["", "All semantic statuses are `UNKNOWN` until manual reverse-engineering and runtime validation."])
    return "\n".join(lines) + "\n"


def print_summary(report: dict[str, Any]) -> None:
    exe = report["executable"]
    print("Crimson Desert Trinity Signature Audit")
    print(f"Executable: {exe['name']}")
    print(f"File version: {exe['fileVersion'] or 'unavailable'}")
    print(f"SHA256: {exe['sha256']}")
    print(f"Known Trinity baseline: {report['baseline']['label']}")
    print()
    for signature_id, result in report["signatures"].items():
        shown = ", ".join(location["rva"] for location in result["locations"][:8])
        suffix = " ..." if result["matches"] > 8 else ""
        print(f"{signature_id}: {result['matches']} [{result['countStatus']}] {shown}{suffix}")


def output_paths(prefix: Path) -> tuple[Path, Path]:
    # Append instead of with_suffix(): versioned names such as 2.01.00-signatures
    # contain dots that pathlib would otherwise mistake for a file extension.
    return Path(f"{prefix}.json"), Path(f"{prefix}.md")


def main() -> int:
    tool_dir = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path, help="Path to CrimsonDesert.exe")
    parser.add_argument("--game-version", default="unknown", help="Reported game version")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=tool_dir / "signatures-2.00.00.json",
        help="Signature baseline manifest",
    )
    parser.add_argument(
        "--output-prefix",
        type=Path,
        help="Write <prefix>.json and <prefix>.md in addition to console output",
    )
    args = parser.parse_args()

    if not args.executable.is_file():
        parser.error(f"executable not found: {args.executable}")
    if not args.manifest.is_file():
        parser.error(f"manifest not found: {args.manifest}")

    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        report = audit(args.executable, manifest, args.game_version)
    except (AuditError, KeyError, json.JSONDecodeError, OSError) as exc:
        parser.error(str(exc))

    print_summary(report)
    if args.output_prefix:
        args.output_prefix.parent.mkdir(parents=True, exist_ok=True)
        json_path, markdown_path = output_paths(args.output_prefix)
        json_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        markdown_path.write_text(markdown_report(report), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
