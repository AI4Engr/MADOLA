Add-Type -AssemblyName System.Drawing

$bmp = New-Object System.Drawing.Bitmap(1024, 1024)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::FromArgb(74, 144, 226))

$font = New-Object System.Drawing.Font("Arial", 512, [System.Drawing.FontStyle]::Bold)
$brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$g.DrawString("M", $font, $brush, 256, 128)

$g.Dispose()
$bmp.Save("$PSScriptRoot\app-icon.png", [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

Write-Host "Icon created successfully at $PSScriptRoot\app-icon.png"