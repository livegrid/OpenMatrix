# Upload firmware then open serial monitor. Kills any lingering pio device monitor so COM port is free.
Set-Location $PSScriptRoot\..

# Free the serial port: kill any pio device monitor processes left from earlier runs
$monitors = Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*device monitor*' }
foreach ($p in $monitors) { Stop-Process -Id $p.ProcessId -Force -ErrorAction SilentlyContinue }
if ($monitors) { Start-Sleep -Seconds 1 }

Write-Host "Uploading firmware..."
& pio run -t upload
if ($LASTEXITCODE -ne 0) {
    Write-Host "Upload failed. Close Serial Monitor or any app using COM9, then run this script again."
    exit $LASTEXITCODE
}

Write-Host "Opening serial monitor (115200). Ctrl+C to exit."
& pio device monitor -b 115200
