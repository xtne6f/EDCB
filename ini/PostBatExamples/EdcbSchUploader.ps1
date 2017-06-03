# EDCBの予約をしょぼいカレンダーにアップロードするスクリプト
# ・仕様は基本的にtvrockSchUploader.jsなどと同じ
# ・以前の送信内容を.lastファイルに書き込み、変化がなければ次回の送信は行わない
# ・スクリプトと同じ場所にEpgTimer.exeが必要
# ・PowerShell3.0以上(EpgTimer.exeが.NET 4系のため)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

# 使用予定チューナーの配色
$DEV_COLORS = "T9999=#ff0000`t#1e90ff"
# ユーザーエージェント
$USER_AGENT = "edcbSchUploader/1.0.0"
# アップロード先
$UPLOAD_URL = "http://cal.syoboi.jp/sch_upload"
# 通信タイムアウト(ミリ秒)
$TIMEOUT = 10000
# 番組名を省略する文字数
$UPLOAD_TITLE_MAXLEN = 30
# 録画優先度がこれ以上の予約だけ送信する
$UPLOAD_REC_PRIORITY = 1

if ($args.Length -lt 2) {
    "Usage: EdcbSchUploader <user> <pass> [epgurl] [slot]"
    exit 2
}
trap {
    $Error[0] | Format-List
    exit 1
}

# サービス名としょぼかる放送局名の対応ファイル"SyoboiCh.txt"があれば読み込む
$chMap = @{}
$chTxtPath = Join-Path (Split-Path $MyInvocation.MyCommand.Path -Parent) SyoboiCh.txt
if (Test-Path $chTxtPath) {
    Get-Content $chTxtPath | Select-String "^[^;].*\t" | %{$chMap[($_ -split "`t")[0]] = ($_ -split "`t")[1]}
}

# EpgTimerのCtrlCmdUtilオブジェクトを生成
$epgTimerExePath = Join-Path (Split-Path $MyInvocation.MyCommand.Path -Parent) EpgTimer.exe
[void][Reflection.Assembly]::LoadFile($epgTimerExePath)
$cmd = New-Object EpgTimer.CtrlCmdUtil

# EpgTimerSrvから予約情報を取得
$rl = New-Object Collections.Generic.List[EpgTimer.ReserveData]
$tl = New-Object Collections.Generic.List[EpgTimer.TunerReserveInfo]
if ($cmd.SendEnumReserve([ref]$rl) -ne [EpgTimer.ErrCode]::CMD_SUCCESS -or
    $cmd.SendEnumTunerReserve([ref]$tl) -ne [EpgTimer.ErrCode]::CMD_SUCCESS) {
    throw "Failed to communicate with EpgTimerSrv"
}
$data = @()
foreach ($r in $rl) {
    $tuner = $null
    foreach ($tr in $tl) {
        if ($tr.reserveList -contains $r.ReserveID) {
            # T+{BonDriver番号(上2桁)}+{チューナ番号(下2桁)},チューナ不足はT9999
            $tuner = "T" + ("" + (10000 + [Math]::Min([Math]::Truncate($tr.tunerID / 65536), 99) * 100 + [Math]::Min($tr.tunerID % 65536, 99))).Substring(1)
            break
        }
    }
    if ($tuner -and $r.RecSetting.RecMode -le 3 -and $r.RecSetting.Priority -ge $UPLOAD_REC_PRIORITY) {
        # "最大200行"
        if ($data.Length -ge 200) {
            break
        }
        # "開始時間 終了時間 デバイス名 番組名 放送局名 サブタイトル オフセット XID"
        $title = $r.Title -replace "[\t\r\n]"
        if ($title.Length -gt $UPLOAD_TITLE_MAXLEN) {
            $title = $title.Substring(0, $UPLOAD_TITLE_MAXLEN - 1) + "…"
        }
        $stationName = $r.StationName
        if ($chMap[$stationName] -ne $null) {
            $stationName = $chMap[$stationName]
        }
        $data += "" +
            ($r.StartTime - (Get-Date -Date 1970-01-01T00:00:00Z)).TotalSeconds + "`t" +
            ($r.StartTime.AddSeconds($r.DurationSecond) - (Get-Date -Date 1970-01-01T00:00:00Z)).TotalSeconds + "`t" +
            "$tuner`t$title`t$stationName`t`t0`t" + $r.ReserveID
    }
}

# 以前の内容と同じなら送信しない
$lastPath = [IO.Path]::ChangeExtension($MyInvocation.MyCommand.Path, "last")
if ((Test-Path $lastPath) -and ($data -join "`n") -ceq ((Get-Content $lastPath) -join "`n")) {
    "Not modified, nothing to do."
    exit 0
}

# 予約情報をサーバに送信
[Net.HttpWebRequest]$wr = [Net.WebRequest]::Create($UPLOAD_URL)
# しょぼかるのPOSTは"Expect: 100-Continue"に対応してないっぽい
$wr.ServicePoint.Expect100Continue = $false
# HttpWebRequestはデフォルトでIEのプロキシ設定に従う。必要であれば$wr.Proxyをここで設定すること
$wr.Timeout = $TIMEOUT
$wr.Credentials = New-Object Net.NetworkCredential ([Uri]::EscapeDataString($args[0]), [Uri]::EscapeDataString($args[1]))
$wr.ContentType = "application/x-www-form-urlencoded"
$wr.UserAgent = $USER_AGENT
$wr.Method = "POST"
$body = "slot="
if ($args.Length -ge 4) {
    $body += [Uri]::EscapeDataString($args[3])
} else {
    $body += "0"
}
$body += "&devcolors=" + [Uri]::EscapeDataString($DEV_COLORS)
$body += "&epgurl="
if ($args.Length -ge 3) {
    $body += [Uri]::EscapeDataString($args[2])
}
$body += "&data=" + [Uri]::EscapeDataString($data -join "`n")
$body = [Text.Encoding]::UTF8.GetBytes($body)

$rs = $wr.GetRequestStream()
$rs.Write($body, 0, $body.Length)
$rs.Close()
$st = [int]$wr.GetResponse().StatusCode
if ($st -lt 200 -or $st -ge 300) {
    throw "Response Error ($st)"
}
Set-Content -Encoding UTF8 $lastPath $data
"Done."
exit 0
