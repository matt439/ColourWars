# GDI/System.Drawing art recipes are qualified under Windows PowerShell 5.1.
if ($PSVersionTable.PSEdition -eq 'Core') {
    throw 'Art builds require Windows PowerShell 5.1; run powershell -NoProfile -File <script>.'
}

# Embedded alongside each art class so separate scripts can share one process.
function Get-ArtPngSource([string] $ClassName) {
    if ($ClassName -cnotmatch '^[A-Za-z][A-Za-z0-9]*$') { throw 'Invalid art PNG helper class name.' }
    return @'
namespace ColourWars {
    internal static class ART_PNG_CLASS {
        public static void SavePng(Bitmap bitmap, string path) {
            using (var encoded = new MemoryStream()) {
                bitmap.Save(encoded, ImageFormat.Png);
                byte[] png = encoded.ToArray();
                using (var output = File.Create(path)) {
                    output.Write(png, 0, 8);
                    for (int offset = 8; offset < png.Length;) {
                        int length = (png[offset] << 24) | (png[offset+1] << 16) |
                            (png[offset+2] << 8) | png[offset+3];
                        string kind = Encoding.ASCII.GetString(png, offset+4, 4);
                        if (kind != "sRGB" && kind != "gAMA" && kind != "iCCP" && kind != "cHRM")
                            output.Write(png, offset, length+12);
                        offset += length+12;
                    }
                }
            }
        }
    }
}
'@.Replace('ART_PNG_CLASS', $ClassName)
}
