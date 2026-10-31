# Dot-sourced by check-paint-fx.ps1 and check-debug-console/driver.ps1, which
# compile System.Drawing code with Add-Type: loads System.Drawing and sets
# $DrawingReferences for -ReferencedAssemblies, so they run under pwsh too.
# The build-*-art.ps1 scripts stay on Windows PowerShell: under pwsh their
# ConvertTo-Json and PNG encoder would rewrite every tracked output.
# Windows PowerShell's mscorlib and System.Drawing hold every type they use;
# pwsh's .NET splits them across reference assemblies and declares the GDI+
# types against private Windows assemblies whose names vary by version.
Add-Type -AssemblyName System.Drawing
$DrawingReferences = @('System.Drawing')
if ($PSVersionTable.PSEdition -eq 'Core') {
    $DrawingReferences = @('System.Collections', 'System.Text.RegularExpressions',
        'System.Threading.Thread', 'System.Drawing.Common', 'System.Drawing.Primitives') +
        @([System.Drawing.Bitmap].Assembly.GetReferencedAssemblies() |
            Where-Object { $_.Name -like 'System.Private.Windows*' } |
            ForEach-Object { [Reflection.Assembly]::Load($_).Location })
}
