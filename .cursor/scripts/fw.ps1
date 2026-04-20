#Requires -Version 5.1
<#
.SYNOPSIS
  Agent automation for PlatformIO: clean, build-only, upload (upload compiles—do not run build before upload),
  timed serial monitor, free serial port. Intended for Cursor/agent use (see AGENTS.md); not the project’s primary human-facing CLI.

.EXAMPLE
  pwsh -File .cursor/scripts/fw.ps1 build
  pwsh -File .cursor/scripts/fw.ps1 upload
  pwsh -File .cursor/scripts/fw.ps1 monitor -Seconds 15
  pwsh -File .cursor/scripts/fw.ps1 free-serial
#>
param(
    [Parameter(Position = 0, Mandatory = $true)]
    [ValidateSet('clean', 'build', 'upload', 'monitor', 'free-serial')]
    [string]$Command,

    [int]$Seconds = 15,

    [string]$Port = ''
)

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    # Script lives at repo/.cursor/scripts/fw.ps1
    return (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}

function Get-PioExe {
    $cmd = Get-Command pio -ErrorAction SilentlyContinue
    if (-not $cmd) {
        Write-Error "pio not found on PATH. Install PlatformIO Core or activate the environment where pio is available."
    }
    return $cmd.Source
}

function Invoke-PioMonitorTimed {
    param([int]$DurationSec, [string]$SerialPort)

    $repo = Get-RepoRoot
    $pio = Get-PioExe
    $argList = @('device', 'monitor')
    if ($SerialPort) {
        $argList += '-p', $SerialPort
    }

    Write-Host "Serial monitor for ${DurationSec}s (then stop). Repo: $repo" -ForegroundColor Cyan

    $p = Start-Process -FilePath $pio -ArgumentList $argList -WorkingDirectory $repo -PassThru -NoNewWindow
    if (-not $p) {
        Write-Error "Failed to start pio device monitor."
    }

    $deadline = (Get-Date).AddSeconds($DurationSec)
    while (-not $p.HasExited -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 200
    }

    if (-not $p.HasExited) {
        Write-Host "`nStopping monitor after ${DurationSec}s." -ForegroundColor Cyan
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 400
    }
}

function Stop-SerialMonitorProcesses {
    $patterns = @(
        '(?i)\bdevice\s+monitor\b',
        '(?i)-m\s+platformio\b.*\bdevice\s+monitor\b'
    )
    $names = @('pio.exe', 'platformio.exe', 'python.exe', 'python3.exe')
    $stopped = 0

    foreach ($name in $names) {
        $procs = Get-CimInstance Win32_Process -Filter "Name = '$name'" -ErrorAction SilentlyContinue
        foreach ($wp in $procs) {
            $line = $wp.CommandLine
            if (-not $line) { continue }
            $match = $false
            foreach ($re in $patterns) {
                if ($line -match $re) { $match = $true; break }
            }
            if (-not $match) { continue }

            $preview = if ($line.Length -le 120) { $line } else { $line.Substring(0, 120) + '...' }
            Write-Host "Stopping PID $($wp.ProcessId) ($name): $preview"
            Stop-Process -Id $wp.ProcessId -Force -ErrorAction SilentlyContinue
            $stopped++
        }
    }

    if ($stopped -eq 0) {
        Write-Host "No matching serial monitor processes found (pio/platformio/python running 'device monitor')."
    } else {
        Write-Host "Stopped $stopped process(es)."
    }
}

$repoRoot = Get-RepoRoot
Set-Location $repoRoot

switch ($Command) {
    'clean' {
        $pio = Get-PioExe
        & $pio run -t clean
        exit $LASTEXITCODE
    }
    'build' {
        $pio = Get-PioExe
        & $pio run
        exit $LASTEXITCODE
    }
    'upload' {
        $pio = Get-PioExe
        & $pio run -t upload
        exit $LASTEXITCODE
    }
    'monitor' {
        if ($Seconds -lt 1) { $Seconds = 15 }
        Invoke-PioMonitorTimed -DurationSec $Seconds -SerialPort $Port
        exit 0
    }
    'free-serial' {
        Stop-SerialMonitorProcesses
        exit 0
    }
}
