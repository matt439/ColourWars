"""A directly owned Windows process, assigned to its kill-on-close job at creation.

PROC_THREAD_ATTRIBUTE_JOB_LIST closes the launch/assignment race. The private
job handle is never inherited; only duplicated stdin/log handles reach a child.
API: https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
"""
import os
import subprocess
import threading
import time


def start_process(arguments, log, environment):
    if os.name != 'nt':
        return subprocess.Popen(arguments, stdout=log, stderr=subprocess.STDOUT, env=environment)
    return WindowsJobProcess(arguments, log, environment)


class WindowsJobProcess:
    def __init__(self, arguments, log, environment):
        import ctypes as c
        from ctypes import wintypes as w
        import msvcrt

        self.arguments = arguments
        self._mutex = threading.RLock()
        self._process = self._job = None
        self.returncode = None
        self._c = c
        self._kernel = k = c.WinDLL('kernel32', use_last_error=True)
        handle, pointer, size = w.HANDLE, c.c_void_p, c.c_size_t

        class BasicLimits(c.Structure):
            _fields_ = [('process_time', c.c_int64), ('job_time', c.c_int64),
                        ('flags', w.DWORD), ('min_working_set', size), ('max_working_set', size),
                        ('active_processes', w.DWORD), ('affinity', size),
                        ('priority', w.DWORD), ('scheduling', w.DWORD)]

        class ExtendedLimits(c.Structure):
            _fields_ = [('basic', BasicLimits), ('io_counters', c.c_uint64 * 6),
                        ('process_memory', size), ('job_memory', size),
                        ('peak_process_memory', size), ('peak_job_memory', size)]

        class StartupInfo(c.Structure):
            _fields_ = [('cb', w.DWORD), ('reserved', w.LPWSTR), ('desktop', w.LPWSTR),
                        ('title', w.LPWSTR), ('x', w.DWORD), ('y', w.DWORD),
                        ('width', w.DWORD), ('height', w.DWORD), ('chars_x', w.DWORD),
                        ('chars_y', w.DWORD), ('fill', w.DWORD), ('flags', w.DWORD),
                        ('show', w.WORD), ('reserved_size', w.WORD), ('reserved_data', pointer),
                        ('stdin', handle), ('stdout', handle), ('stderr', handle)]

        class StartupInfoEx(c.Structure):
            _fields_ = [('startup', StartupInfo), ('attributes', pointer)]

        class ProcessInfo(c.Structure):
            _fields_ = [('process', handle), ('thread', handle), ('pid', w.DWORD), ('tid', w.DWORD)]

        signatures = {
            'CreateJobObjectW': ([pointer, w.LPCWSTR], handle),
            'SetInformationJobObject': ([handle, c.c_int, pointer, w.DWORD], w.BOOL),
            'CloseHandle': ([handle], w.BOOL),
            'GetCurrentProcess': ([], handle),
            'DuplicateHandle': ([handle, handle, handle, c.POINTER(handle), w.DWORD, w.BOOL, w.DWORD], w.BOOL),
            'InitializeProcThreadAttributeList': ([pointer, w.DWORD, w.DWORD, c.POINTER(size)], w.BOOL),
            'UpdateProcThreadAttribute': ([pointer, w.DWORD, size, pointer, size, pointer, pointer], w.BOOL),
            'DeleteProcThreadAttributeList': ([pointer], None),
            'CreateProcessW': ([w.LPCWSTR, w.LPWSTR, pointer, pointer, w.BOOL, w.DWORD,
                                pointer, w.LPCWSTR, pointer, c.POINTER(ProcessInfo)], w.BOOL),
            'WaitForSingleObject': ([handle, w.DWORD], w.DWORD),
            'GetExitCodeProcess': ([handle, c.POINTER(w.DWORD)], w.BOOL),
            'TerminateProcess': ([handle, w.UINT], w.BOOL),
        }
        for name, (argtypes, restype) in signatures.items():
            function = getattr(k, name)
            function.argtypes, function.restype = argtypes, restype

        def require(result):
            if not result:
                raise c.WinError(c.get_last_error())
            return result

        inherited = []
        attributes = None
        attributes_ready = False
        info = ProcessInfo()
        try:
            self._job = require(k.CreateJobObjectW(None, None))
            limits = ExtendedLimits()
            limits.basic.flags = 0x2000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
            require(k.SetInformationJobObject(self._job, 9, c.byref(limits), c.sizeof(limits)))
            current = k.GetCurrentProcess()
            with open(os.devnull, 'rb') as null:
                for stream in (null, log):
                    duplicate = handle()
                    require(k.DuplicateHandle(current, msvcrt.get_osfhandle(stream.fileno()),
                                              current, c.byref(duplicate), 0, True, 2))
                    inherited.append(duplicate.value)
            byte_count = size()
            # This sizing query fails by design and fills the required size.
            k.InitializeProcThreadAttributeList(None, 2, 0, c.byref(byte_count))
            if not byte_count.value:
                raise c.WinError(c.get_last_error())
            attributes = c.create_string_buffer(byte_count.value)
            require(k.InitializeProcThreadAttributeList(attributes, 2, 0, c.byref(byte_count)))
            attributes_ready = True
            jobs = (handle * 1)(self._job)
            handles = (handle * len(inherited))(*inherited)
            require(k.UpdateProcThreadAttribute(attributes, 0, 0x2000D, jobs, c.sizeof(jobs), None, None))
            require(k.UpdateProcThreadAttribute(attributes, 0, 0x20002, handles, c.sizeof(handles), None, None))
            startup = StartupInfoEx()
            startup.startup.cb = c.sizeof(startup)
            startup.startup.flags = 0x100  # STARTF_USESTDHANDLES
            startup.startup.stdin = inherited[0]
            startup.startup.stdout = startup.startup.stderr = inherited[1]
            startup.attributes = c.cast(attributes, pointer)
            command = c.create_unicode_buffer(subprocess.list2cmdline([str(a) for a in arguments]))
            block = c.create_unicode_buffer('\0'.join(f'{key}={value}' for key, value in
                                           sorted(environment.items(), key=lambda item: item[0].upper())) + '\0\0')
            require(k.CreateProcessW(str(arguments[0]), command, None, None, True,
                                     0x80000 | 0x400,  # EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT
                                     block, None, c.byref(startup), c.byref(info)))
            self._process, self.pid = info.process, info.pid
        except BaseException:
            if self._job:
                k.CloseHandle(self._job)
                self._job = None
            if info.process:
                k.WaitForSingleObject(info.process, 5000)
                k.CloseHandle(info.process)
                self._process = None
            raise
        finally:
            if info.thread:
                k.CloseHandle(info.thread)
            if attributes_ready:
                k.DeleteProcThreadAttributeList(attributes)
            for inherited_handle in inherited:
                k.CloseHandle(inherited_handle)

    def _observe(self, milliseconds):
        if self.returncode is not None:
            return self.returncode
        result = self._kernel.WaitForSingleObject(self._process, milliseconds)
        if result == 258:  # WAIT_TIMEOUT
            return None
        if result != 0:
            raise self._c.WinError(self._c.get_last_error())
        from ctypes import wintypes
        code = wintypes.DWORD()
        if not self._kernel.GetExitCodeProcess(self._process, self._c.byref(code)):
            raise self._c.WinError(self._c.get_last_error())
        self.returncode = code.value
        self._kernel.CloseHandle(self._process)
        self._kernel.CloseHandle(self._job)
        self._process = self._job = None
        return self.returncode

    def poll(self):
        with self._mutex:
            return self._observe(0)

    def wait(self, timeout=None):
        deadline = None if timeout is None else time.monotonic() + timeout
        while True:
            remaining = None if deadline is None else deadline - time.monotonic()
            with self._mutex:
                result = self._observe(100 if remaining is None else max(0, min(100, int(remaining * 1000))))
            if result is not None:
                return result
            if remaining is not None and remaining <= 0:
                raise subprocess.TimeoutExpired(self.arguments, timeout)

    def terminate(self):
        with self._mutex:
            if self._observe(0) is None and not self._kernel.TerminateProcess(self._process, 1):
                # A process can exit between observation and termination.
                if self._observe(0) is None:
                    raise self._c.WinError(self._c.get_last_error())
