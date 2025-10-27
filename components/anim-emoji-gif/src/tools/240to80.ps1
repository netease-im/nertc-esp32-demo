# 放在仓库根目录执行
param(
    [string]$inDir  = "240",
    [string]$outDir = "80"
)

# 创建输出目录
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Force $outDir }

Get-ChildItem "$inDir\*.gif" | ForEach-Object {
    $outFile = "$outDir\$($_.Name)"
    magick $_.FullName `
        -coalesce `
        -resize 80x80 `
        -colors 32 -dither FloydSteinberg `
        -strip `
        -layers OptimizeTransparency `
        "$outFile"
    Write-Host "已生成 $outFile"
}