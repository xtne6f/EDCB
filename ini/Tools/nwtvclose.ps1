# Call SendNwTVIDClose()

$ErrorActionPreference = 'Stop'

$nwTVID = [int]$args[0]
$openID = $args[1]

if ($nwTVID -lt 0 -or $nwTVID -gt 65535) {
    throw 'Invalid argument'
}

if ($null -eq $openID) {
    $sb = $null
} else {
    Add-Type -Name NativeMethods -Namespace Win32 -MemberDefinition '[DllImport("kernel32.dll", CharSet = CharSet.Unicode)] public static extern uint GetPrivateProfileString(string app, string key, string def, System.Text.StringBuilder sb, uint n, string name);'

    $sb = New-Object Text.StringBuilder 1024
    $null = [Win32.NativeMethods]::GetPrivateProfileString('SET', 'DataSavePath', '', $sb, $sb.Capacity, $PSScriptRoot + '\..\Common.ini')
    $dataSavePath = [string]$sb
    if ($dataSavePath -eq '') {
        $dataSavePath = $PSScriptRoot + '\..\Setting'
    }
    $sb.Length = 0
    $null = [Win32.NativeMethods]::GetPrivateProfileString('NWTV', 'nwtv' + $nwTVID + 'open', $openID, $sb, $sb.Capacity, $dataSavePath + '\HttpPublic.ini')
}

if ($null -eq $sb -or [string]$sb -eq $openID) {
    "SendNwTVIDClose($nwTVID) ..."

    1..50 | foreach {
        $pipe = New-Object IO.Pipes.NamedPipeClientStream 'EpgTimerSrvNoWaitPipe'
        try {
            $pipe.Connect(0)
            $pipe.Write([byte[]](50, 4, 0, 0, 4, 0, 0, 0, ($nwTVID % 256), [Math]::Floor($nwTVID / 256), 0, 0), 0, 12)
            break
        } catch {
        } finally {
            $pipe.Close()
        }
        sleep -m 100
    }
}
