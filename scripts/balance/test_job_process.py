"""Own only the child tree on Windows, including when its driver is killed."""
import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

import job_process


@unittest.skipUnless(os.name == 'nt', 'Windows Job Object ownership')
class JobProcessTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)

    def test_unicode_arguments_environment_log_and_exit(self):
        log_path = self.root / 'output.log'
        environment = dict(os.environ, COLOURWARS_JOB_TEST='paint \u2713', PYTHONIOENCODING='utf-8')
        with log_path.open('wb') as log:
            child = job_process.start_process(
                [sys.executable, '-c', 'import os,sys; print(sys.argv[1]); print(os.environ["COLOURWARS_JOB_TEST"]); sys.exit(7)',
                 'a space and "a quote" \u2713'], log, environment)
            self.assertEqual(child.wait(timeout=10), 7)
            self.assertEqual(child.poll(), 7)
        # Use a Unicode-output encoding independent of the console codepage.
        text = log_path.read_text(encoding='utf-8')
        self.assertIn('a space and "a quote" \u2713', text)
        self.assertIn('paint \u2713', text)

    def test_timeout_then_terminate(self):
        with (self.root / 'timeout.log').open('wb') as log:
            child = job_process.start_process([sys.executable, '-c', 'import time; time.sleep(60)'], log, os.environ.copy())
            try:
                with self.assertRaises(subprocess.TimeoutExpired):
                    child.wait(timeout=0.05)
                self.assertIsNone(child.poll())
            finally:
                child.terminate()
                self.assertEqual(child.wait(timeout=10), 1)

    def test_hard_killed_driver_cleans_child_but_keeps_unrelated_process(self):
        pid_file = self.root / 'child.pid'
        driver_file = self.root / 'driver.py'
        driver_file.write_text(
            'import os,sys,time\nfrom pathlib import Path\n'
            f'sys.path.insert(0,{str(Path(job_process.__file__).parent)!r})\n'
            'from job_process import start_process\n'
            'with open(sys.argv[2],"wb") as log:\n'
            ' child=start_process([sys.executable,"-c","import time;time.sleep(60)"],log,os.environ.copy())\n'
            ' Path(sys.argv[1]).write_text(str(child.pid))\n time.sleep(60)\n', encoding='utf-8')
        sentinel = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])
        owner = subprocess.Popen([sys.executable, str(driver_file), str(pid_file), str(self.root / 'child.log')])
        child_handle = None
        kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel.OpenProcess.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_uint32]
        kernel.OpenProcess.restype = ctypes.c_void_p
        kernel.WaitForSingleObject.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        kernel.WaitForSingleObject.restype = ctypes.c_uint32
        kernel.CloseHandle.argtypes = [ctypes.c_void_p]
        try:
            deadline = time.monotonic() + 10
            while not pid_file.exists() and time.monotonic() < deadline:
                self.assertIsNone(owner.poll(), 'driver failed before creating its child')
                time.sleep(0.02)
            self.assertTrue(pid_file.is_file(), 'driver did not publish the private child PID')
            child_handle = kernel.OpenProcess(0x100000, False, int(pid_file.read_text()))
            self.assertTrue(child_handle, 'could not retain a synchronization handle to the private child')
            owner.terminate()
            owner.wait(timeout=10)
            self.assertEqual(kernel.WaitForSingleObject(child_handle, 10000), 0,
                             'driver death left its child alive')
            self.assertIsNone(sentinel.poll(), 'cleanup killed an unrelated sentinel')
        finally:
            if child_handle:
                kernel.CloseHandle(child_handle)
            for process in (owner, sentinel):
                if process.poll() is None:
                    process.terminate()
                process.wait(timeout=10)


if __name__ == '__main__':
    unittest.main()
