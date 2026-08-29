#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) M. Boerger and the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

import io
import pathlib
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest

from tools import clang_tidy_runner


class ClangTidyRunnerTest(unittest.TestCase):
    def test_progress_line(self):
        result = clang_tidy_runner.Result(
            clang_tidy_runner.Task("mbo/file/glob.cc"), 0, "", 4.25
        )
        self.assertEqual(
            clang_tidy_runner.progress_line(42, 187, result),
            "[ 42/187  22.5%] PASS mbo/file/glob.cc (4.2s)",
        )

    def test_parallel_run_reports_completion_and_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            executable = root / "fake-clang-tidy"
            executable.write_text(
                textwrap.dedent(
                    """\
                    #!/usr/bin/env python3
                    import pathlib
                    import sys
                    import time

                    path = pathlib.Path(sys.argv[-1]).name
                    time.sleep(0.05 if path == "slow.cc" else 0.01)
                    if path == "bad.cc":
                        print("bad.cc:1:1: error: finding")
                        raise SystemExit(1)
                    print(f"{path}: routine successful output")
                    """
                ),
                encoding="utf-8",
            )
            executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            output = root / "diagnostics.txt"
            stream = io.StringIO()
            status = clang_tidy_runner.run_all(
                [
                    clang_tidy_runner.Task("slow.cc"),
                    clang_tidy_runner.Task("good.cc"),
                    clang_tidy_runner.Task("bad.cc"),
                ],
                str(executable),
                ".",
                2,
                str(output),
                stream,
            )

            rendered = stream.getvalue()
            self.assertEqual(status, 1)
            self.assertIn("clang-tidy: 3 translation unit(s), 2 worker(s)", rendered)
            self.assertIn("[1/3  33.3%] PASS good.cc", rendered)
            self.assertIn("PASS slow.cc", rendered)
            self.assertIn("[3/3 100.0%]", rendered)
            self.assertIn("FAIL bad.cc", rendered)
            self.assertIn("clang-tidy: 2 passed, 1 failed", rendered)
            self.assertIn("bad.cc:1:1: error: finding", output.read_text(encoding="utf-8"))
            self.assertNotIn("routine successful output", rendered)
            self.assertIn("routine successful output", output.read_text(encoding="utf-8"))

    def test_registry_terminates_active_children(self):
        registry = clang_tidy_runner.ProcessRegistry()
        process = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(30)"])
        self.assertTrue(registry.add(process))

        registry.terminate_all()

        self.assertIsNotNone(process.poll())
        self.assertFalse(registry.add(process))


if __name__ == "__main__":
    unittest.main()
