"""Compiler probe controls; never install software or produce an installer."""
import importlib.util
import contextlib
import io
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).parents[1] / "scripts/verify_inno_version.py"


class InnoVersionTests(unittest.TestCase):
    def load(self):
        spec = importlib.util.spec_from_file_location("inno_version", SCRIPT)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_real_engine_banner_without_product_metadata(self):
        module = self.load()
        result = subprocess.CompletedProcess([], 0, "Inno Setup 6 Command-Line Compiler\nCompiler engine version: Inno Setup 6.7.1\nSuccessful compile.\n")
        with patch.object(module.subprocess, "run", return_value=result) as run:
            self.assertEqual(module.verify(Path("ISCC.exe"), "6.7.1"), "6.7.1")
        self.assertEqual(run.call_args.args[0], [str(Path("ISCC.exe").resolve()), "/O-", "-"])
        self.assertIn("Output=no", run.call_args.kwargs["input"])
        self.assertFalse(run.call_args.kwargs.get("shell", False))

    def test_wrong_missing_ambiguous_or_failed_probe_rejected(self):
        module = self.load()
        good = "Compiler engine version: Inno Setup 6.7.1\n"
        for code, output in ((0, good.replace("6.7.1", "6.7.2")), (0, "6.7.1"),
                             (0, good + good), (2, good), (0, good.replace("6.7.1", "6.7.1-beta"))):
            with self.subTest(code=code, output=output):
                with patch.object(module.subprocess, "run", return_value=subprocess.CompletedProcess([], code, output)):
                    with self.assertRaises(ValueError):
                        module.verify(Path("ISCC.exe"), "6.7.1")

    def test_launch_failure_and_timeout_do_not_pass(self):
        module = self.load()
        for error in (OSError("missing compiler"), subprocess.TimeoutExpired("ISCC", 60)):
            with patch.object(module.subprocess, "run", side_effect=error):
                with self.assertRaises(type(error)):
                    module.verify(Path("ISCC.exe"), "6.7.1")

    def test_cli_failure_is_nonzero_without_success_output(self):
        module = self.load()
        out, err = io.StringIO(), io.StringIO()
        with patch.object(sys, "argv", [str(SCRIPT), "--inno", "ISCC.exe", "--expected", "6.7.1"]), \
                patch.object(module, "verify", side_effect=ValueError("wrong version")), \
                contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            self.assertEqual(module.main(), 1)
        self.assertEqual(out.getvalue(), "")
        self.assertIn("wrong version", err.getvalue())
