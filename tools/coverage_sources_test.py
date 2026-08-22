#!/usr/bin/env python3
"""Tests for tools/coverage_sources.py."""

import tempfile
import unittest
from pathlib import Path

from tools import coverage_sources


class CoverageSourcesTest(unittest.TestCase):
    def test_groups_selected_sources_by_policy_category(self):
        policy = {
            "include": ["mbo/**"],
            "exclude": ["**/*_test.cc"],
            "categories": {
                "file": {"include": ["mbo/file/**"]},
                "hash": {"include": ["mbo/hash/**"]},
            },
        }
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory) / "workspace"
            (workspace / "mbo/file").mkdir(parents=True)
            (workspace / "mbo/hash").mkdir(parents=True)
            (workspace / "mbo/file/file.cc").touch()
            (workspace / "mbo/hash/hash.h").touch()
            root = Path(directory) / "grouped"
            report = (
                "SF:mbo/file/file.cc\nDA:1,1\nLF:1\nend_of_record\n"
                "SF:mbo/hash/hash.h\nDA:1,1\nLF:1\nend_of_record\n"
                "SF:mbo/file/file_test.cc\nDA:1,1\nLF:1\nend_of_record\n"
                "SF:mbo/config/config.h\nFNF:0\nFNH:0\nLH:0\nLF:0\nend_of_record\n"
            )
            actual = coverage_sources.grouped(report, policy, root, workspace)
            self.assertIn(f"SF:{root.resolve()}/file/mbo/file/file.cc", actual)
            self.assertIn(f"SF:{root.resolve()}/hash/mbo/hash/hash.h", actual)
            self.assertNotIn("file_test.cc", actual)
            self.assertNotIn("config.h", actual)
            self.assertTrue((root / "file/mbo/file/file.cc").is_symlink())

    def test_rejects_sources_in_multiple_categories(self):
        policy = {
            "categories": {
                "all": {"include": ["mbo/**"]},
                "file": {"include": ["mbo/file/**"]},
            }
        }
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "belongs to 2 policy categories"):
                coverage_sources.grouped(
                    "SF:mbo/file/file.cc\nLF:1\nend_of_record\n",
                    policy,
                    Path(directory),
                    Path(directory),
                )


if __name__ == "__main__":
    unittest.main()
