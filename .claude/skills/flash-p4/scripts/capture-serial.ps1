# Capture the display node's serial output while a crash is reproduced.
#
# A reboot drops the USB CDC device and re-enumerates it, which ends a plain
# `pio device monitor`; this reopens the port and keeps going, so the panic
# dump AND the boot banner after it both land in one log.
#
#   .\capture-serial.ps1 -Port COM15 -Seconds 150 -Out crash.log
#
# Find the port with `pio device list --serial` — the board is the device with
# VID:PID=303A:1001.

param(
    [Parameter(Mandatory = $true)][string]$Port,
    [int]$Seconds = 150,
    [string]$Out = 'serial.log'
)

Set-Content -Path $Out -Value '' -Encoding utf8
Write-Output "Capturing $Port for $Seconds s into $Out - reproduce the crash now."

$end = (Get-Date).AddSeconds($Seconds)
$sp = $null

while ((Get-Date) -lt $end) {
    if ($null -eq $sp -or -not $sp.IsOpen) {
        try {
            $sp = New-Object System.IO.Ports.SerialPort($Port, 115200, 'None', 8, 'One')
            # Leave the handshake lines alone: toggling DTR/RTS resets the chip.
            $sp.DtrEnable = $false
            $sp.RtsEnable = $false
            $sp.ReadTimeout = 300
            $sp.Open()
            Add-Content -Path $Out -Value "`n--- port opened $(Get-Date -Format HH:mm:ss) ---" -Encoding utf8
        }
        catch {
            Start-Sleep -Milliseconds 300
            continue
        }
    }
    try {
        $chunk = $sp.ReadExisting()
        if ($chunk) { Add-Content -Path $Out -Value $chunk -NoNewline -Encoding utf8 }
    }
    catch {
        try { $sp.Close() } catch {}
        $sp = $null
    }
    Start-Sleep -Milliseconds 100
}

if ($null -ne $sp -and $sp.IsOpen) { $sp.Close() }
Write-Output "Capture done: $Out"
