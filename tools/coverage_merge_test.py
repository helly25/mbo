#!/usr/bin/env python3
"""Tests for tools/coverage_merge.py."""

import unittest

from tools import coverage_merge


class CoverageMergeTest(unittest.TestCase):
    def test_rejects_an_empty_supplemental_report(self):
        with self.assertRaisesRegex(ValueError, "no source records"):
            coverage_merge.overlay("SF:mbo/json/json.h\nDA:1,0\nend_of_record\n", "")

    def test_rejects_supplemental_data_without_matching_hits(self):
        with self.assertRaisesRegex(ValueError, "no matching production hits"):
            coverage_merge.overlay(
                "SF:mbo/json/json.h\nDA:1,0\nend_of_record\n",
                "SF:mbo/json/json.h\nDA:2,1\nend_of_record\n",
            )

    def test_overlays_only_matching_production_coverpoints(self):
        primary = (
            "SF:mbo/json/json.h\n"
            "FN:10,existing\nFNDA:0,existing\nFNF:1\nFNH:0\n"
            "BRDA:11,0,0,-\nBRDA:11,0,1,0\nBRF:2\nBRH:0\n"
            "DA:10,1,checksum\nDA:11,0\nLF:2\nLH:1\nend_of_record\n"
        )
        supplemental = (
            "SF:/runner/work/mbo/mbo/mbo/json/json.h\n"
            "FN:10,existing\nFN:12,exception_only\n"
            "FNDA:2,existing\nFNDA:4,exception_only\nFNF:2\nFNH:2\n"
            "BRDA:11,0,0,3\nBRDA:12,0,0,4\nBRF:2\nBRH:2\n"
            "DA:10,2,checksum\nDA:11,3\nDA:12,4\nLF:3\nLH:3\nend_of_record\n"
        )

        actual = coverage_merge.overlay(primary, supplemental)

        self.assertIn("FNDA:2,existing\nFNF:1\nFNH:1", actual)
        self.assertNotIn("exception_only", actual)
        self.assertIn("BRDA:11,0,0,3\nBRDA:11,0,1,0\nBRF:2\nBRH:1", actual)
        self.assertNotIn("BRDA:12", actual)
        self.assertIn("DA:10,3,checksum\nDA:11,3\nLF:2\nLH:2", actual)
        self.assertNotIn("DA:12", actual)


if __name__ == "__main__":
    unittest.main()
