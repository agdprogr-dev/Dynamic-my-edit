"""Scoped subprocess regressions for verifier execution modes (requires Unicorn)."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERIFIER = ROOT / 'tools' / 'verify_inputalign_80.py'
ORIGINAL = ROOT / 'InputAlign-v2.7.1-alpha.1-windows.geode'
PATCHED = ROOT / 'InputAlign-v2.7.1-alpha.1-80ticks-windows.geode'
ERROR = 'Verification cannot run with Python optimization; remove -O/-OO and PYTHONOPTIMIZE'


class VerificationModeTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.invalid = Path(self.scratch.name) / 'invalid.geode'
        with zipfile.ZipFile(PATCHED) as source, zipfile.ZipFile(self.invalid, 'w') as target:
            for entry in source.infolist():
                data = source.read(entry.filename)
                if entry.filename == 'mod.json':
                    meta = json.loads(data)
                    meta['settings']['normal-ticks']['default'] = 40
                    data = json.dumps(meta).encode()
                target.writestr(entry, data)

    def run_verifier(self, package, flags=(), optimize=None, imported=False):
        env = os.environ.copy()
        env.pop('PYTHONOPTIMIZE', None)
        env['PYTHONDONTWRITEBYTECODE'] = '1'
        if optimize is not None:
            env['PYTHONOPTIMIZE'] = optimize
        if imported:
            command = ['-c', 'from verify_inputalign_80 import verify; import sys; verify(*sys.argv[1:])']
        else:
            command = [str(VERIFIER)]
        return subprocess.run([sys.executable, *flags, *command, str(ORIGINAL), str(package)],
                              cwd=VERIFIER.parent, env=env, text=True, capture_output=True, timeout=30)

    def assert_rejected_optimization(self, result):
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(ERROR, result.stderr)
        self.assertNotIn('PASS:', result.stdout)

    def test_normal_valid_package_runs_all_checks(self):
        result = self.run_verifier(PATCHED)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('108 isolated x64 cases', result.stdout)

    def test_normal_invalid_metadata_cannot_pass(self):
        result = self.run_verifier(self.invalid)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('AssertionError', result.stderr)
        self.assertNotIn('PASS:', result.stdout)

    def test_optimization_flags_rejected(self):
        for flag in ('-O', '-OO'):
            for package in (PATCHED, self.invalid):
                with self.subTest(flag=flag, package=package.name):
                    self.assert_rejected_optimization(self.run_verifier(package, flags=(flag,)))

    def test_optimization_environment_rejected(self):
        for value in ('1', '2'):
            for package in (PATCHED, self.invalid):
                with self.subTest(value=value, package=package.name):
                    self.assert_rejected_optimization(self.run_verifier(package, optimize=value))

    def test_direct_import_rejected_before_opening_packages(self):
        result = self.run_verifier(Path(self.scratch.name) / 'missing.geode', flags=('-O',), imported=True)
        self.assert_rejected_optimization(result)


if __name__ == '__main__':
    unittest.main()
