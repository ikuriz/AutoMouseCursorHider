Add-Type -AssemblyName System.Drawing

$source = Join-Path $PSScriptRoot 'app-source.png'
$output = Join-Path $PSScriptRoot 'AutoMouseCursorHider.ico'
$sizes = @(16, 24, 32, 48, 64, 128, 256)
$sourceImage = [System.Drawing.Image]::FromFile($source)
$frameData = @()

# Detect the artwork bounds and keep a small square margin around the subject.
$sourceBitmap = New-Object System.Drawing.Bitmap($sourceImage)
$minX = $sourceBitmap.Width; $minY = $sourceBitmap.Height
$maxX = -1; $maxY = -1
for ($y = 0; $y -lt $sourceBitmap.Height; $y += 2) {
    for ($x = 0; $x -lt $sourceBitmap.Width; $x += 2) {
        $pixel = $sourceBitmap.GetPixel($x, $y)
        if ($pixel.R -lt 245 -or $pixel.G -lt 245 -or $pixel.B -lt 245) {
            if ($x -lt $minX) { $minX = $x }
            if ($y -lt $minY) { $minY = $y }
            if ($x -gt $maxX) { $maxX = $x }
            if ($y -gt $maxY) { $maxY = $y }
        }
    }
}
$sourceBitmap.Dispose()
$contentWidth = $maxX - $minX + 1
$contentHeight = $maxY - $minY + 1
$cropSize = [Math]::Ceiling([Math]::Max($contentWidth, $contentHeight) * 1.10)
$centerX = ($minX + $maxX) / 2
$centerY = ($minY + $maxY) / 2
$cropLeft = [Math]::Max(0, [Math]::Min($sourceImage.Width - $cropSize, $centerX - ($cropSize / 2)))
$cropTop = [Math]::Max(0, [Math]::Min($sourceImage.Height - $cropSize, $centerY - ($cropSize / 2)))
$cropRect = [System.Drawing.Rectangle]::new([int]$cropLeft, [int]$cropTop, [int]$cropSize, [int]$cropSize)

try {
    foreach ($size in $sizes) {
        $bitmap = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([System.Drawing.Color]::Transparent)
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $graphics.DrawImage($sourceImage, [System.Drawing.Rectangle]::new(0, 0, $size, $size),
                                $cropRect, [System.Drawing.GraphicsUnit]::Pixel)
        }
        finally {
            $graphics.Dispose()
        }

        # Write a classic Windows DIB icon frame. This is more compatible with
        # Explorer and taskbar icon extraction than PNG-compressed ICO frames.
        $frameStream = New-Object System.IO.MemoryStream
        $frameWriter = New-Object System.IO.BinaryWriter($frameStream)
        $frameWriter.Write([uint32]40)
        $frameWriter.Write([int32]$size)
        $frameWriter.Write([int32]($size * 2))
        $frameWriter.Write([uint16]1)
        $frameWriter.Write([uint16]32)
        $frameWriter.Write([uint32]0)
        $frameWriter.Write([uint32]($size * $size * 4))
        $frameWriter.Write([int32]0)
        $frameWriter.Write([int32]0)
        $frameWriter.Write([uint32]0)
        $frameWriter.Write([uint32]0)
        for ($row = $size - 1; $row -ge 0; $row--) {
            for ($column = 0; $column -lt $size; $column++) {
                $pixel = $bitmap.GetPixel($column, $row)
                $frameWriter.Write([byte]$pixel.B)
                $frameWriter.Write([byte]$pixel.G)
                $frameWriter.Write([byte]$pixel.R)
                $frameWriter.Write([byte]$pixel.A)
            }
        }
        $maskRowBytes = [int]([Math]::Ceiling($size / 32.0) * 4)
        $mask = New-Object byte[] $maskRowBytes
        for ($row = 0; $row -lt $size; $row++) { $frameWriter.Write($mask) }
        $frameWriter.Dispose()
        $frameData += ,$frameStream.ToArray()
        $frameStream.Dispose()
        $bitmap.Dispose()
    }
}
finally {
    $sourceImage.Dispose()
}

$outputStream = New-Object System.IO.FileStream($output, [System.IO.FileMode]::Create)
$writer = New-Object System.IO.BinaryWriter($outputStream)
try {
    $writer.Write([uint16]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]$sizes.Count)
    $offset = 6 + (16 * $sizes.Count)
    for ($index = 0; $index -lt $sizes.Count; $index++) {
        $size = $sizes[$index]
        $data = $frameData[$index]
        $dimension = if ($size -eq 256) { 0 } else { $size }
        $writer.Write([byte]$dimension)
        $writer.Write([byte]$dimension)
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]32)
        $writer.Write([uint32]$data.Length)
        $writer.Write([uint32]$offset)
        $offset += $data.Length
    }
    foreach ($data in $frameData) { $writer.Write($data) }
}
finally {
    $writer.Dispose()
    $outputStream.Dispose()
}

Write-Output "Created $output"
