"""Source wiring guards; native pixel/PowerShell acceptance remains on Windows."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class HostedVisualPolicy(unittest.TestCase):
    def test_nightly_explicitly_selects_every_visual_entry(self):
        workflow = (ROOT / '.github/workflows/nightly.yml').read_text()
        for entry, count in [('check-paint-fx.ps1', 1), ('check-painter.ps1', 1),
                             ('check-painter-sweep.ps1', 2)]:
            self.assertEqual(workflow.count(f'-File scripts/{entry} -HostedSoftwareFunctional -Preset'), count)

    def test_sweep_and_second_process_forward_policy(self):
        sweep = (ROOT / 'scripts/check-painter-sweep.ps1').read_text()
        self.assertIn("if ($HostedSoftwareFunctional) { $adapterArguments += '-HostedSoftwareFunctional' }", sweep)
        self.assertIn('-SkipBuild "-$leaf" @adapterArguments', sweep)
        painter = (ROOT / 'scripts/check-painter.ps1').read_text()
        for arguments in ['probeArguments', 'readerArguments']:
            self.assertIn(f"if ($HostedSoftwareFunctional) {{ ${arguments} += '--hosted-software-functional' }}", painter)

    def test_each_native_policy_rejects_wrong_and_unknown_devices(self):
        for name in ['painter_visual', 'paint_fx_visual']:
            source = (ROOT / f'scripts/acceptance/{name}.cpp').read_text()
            self.assertIn('device.backend == "d3d11" && admits_adapter(device.kind, hosted_software)', source)
            for kind, hosted, accepted in [('hardware', False, True), ('software', False, False),
                                          ('hardware', True, False), ('software', True, True),
                                          ('null_device', False, False), ('null_device', True, False)]:
                prefix = '' if accepted else '!'
                self.assertIn(f'static_assert({prefix}admits_adapter(RenderDeviceKind::{kind}, {str(hosted).lower()}));', source)
            self.assertIn('std::wstring(argv[argc - 1]) == L"--hosted-software-functional"', source)
            self.assertIn('if (hosted_software) --argc;', source)
            self.assertIn('"ADAPTER_POLICY "', source)

    def test_wrapper_policy_validation_preserves_failure_diagnostic(self):
        for name, count in [('check-painter.ps1', 2), ('check-paint-fx.ps1', 1)]:
            source = (ROOT / f'scripts/{name}').read_text()
            self.assertEqual(source.count('$policyLines.Count -ne 1'), count)
            self.assertEqual(source.count('$deviceLines.Count -ne 1'), count)
            segments = source.split('$runLines = @(Get-Content')[1:]
            self.assertEqual(len(segments), count)
            for segment in segments:
                self.assertLess(segment.index('if ($process.ExitCode -ne 0)'), segment.index('$deviceLines.Count -ne 1'))
                self.assertIn('$policyLines[0] -cne "ADAPTER_POLICY $($result.adapter_policy)"', segment)

    def test_receipts_distinguish_functional_from_hardware_acceptance(self):
        for path in ['scripts/check-painter/evidence-record.ps1', 'scripts/check-paint-fx.ps1',
                     'scripts/check-painter-sweep.ps1']:
            source = (ROOT / path).read_text()
            self.assertIn("adapter_policy = if ($HostedSoftwareFunctional) { 'hosted_software_functional_only' } else { 'hardware_only' }", source)
            self.assertIn('hardware_acceptance = $false', source)


if __name__ == '__main__':
    unittest.main()
