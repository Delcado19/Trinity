import struct
import unittest
from pathlib import Path

import audit


class AuditTests(unittest.TestCase):
    def test_pe_scan_and_wildcard(self) -> None:
        image = bytearray(0x400)
        image[:2] = b"MZ"
        struct.pack_into("<I", image, 0x3C, 0x80)
        image[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<HHIIIHH", image, 0x84, 0x8664, 1, 0x12345678, 0, 0, 0xF0, 0)
        struct.pack_into("<H", image, 0x98, 0x20B)
        struct.pack_into("<I", image, 0x98 + 56, 0x2000)
        section = 0x98 + 0xF0
        image[section : section + 8] = b".text\0\0\0"
        struct.pack_into("<IIII", image, section + 8, 0x200, 0x1000, 0x200, 0x200)
        struct.pack_into("<I", image, section + 36, audit.IMAGE_SCN_MEM_EXECUTE | audit.IMAGE_SCN_MEM_READ)
        image[0x220:0x224] = bytes.fromhex("48 8B 05 90")

        pe = audit.parse_pe(image)
        hits = audit.scan_sections(image, pe.sections, audit.parse_pattern("48 8B ?? 90"))

        self.assertEqual(hits, [{"rva": 0x1020, "section": ".text"}])
        with self.assertRaises(audit.AuditError):
            audit.parse_pattern("?? ?")

    def test_output_paths_append_extensions_to_versioned_prefix(self) -> None:
        prefix = Path("reports/crimson-desert-2.01.00-signatures")
        self.assertEqual(
            audit.output_paths(prefix),
            (Path(f"{prefix}.json"), Path(f"{prefix}.md")),
        )


if __name__ == "__main__":
    unittest.main()
