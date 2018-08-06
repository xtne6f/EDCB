# 追従発生時にメールするスクリプト
# _EDCBX_HIDE_

# Gmailをひな型にしたがなんでもいい
$M_SMTP = 'smtp.gmail.com'
$M_PORT = 587
if (Test-Path .\Tools\mail_credential.txt) {
    $cred = New-Object Management.Automation.PSCredential ((cat .\Tools\mail_credential.txt)[0], (ConvertTo-SecureString (cat .\Tools\mail_credential.txt)[1]))
} else {
    $cred = New-Object Management.Automation.PSCredential ((cat .\Tools\mail_plain.txt)[0], (ConvertTo-SecureString -AsPlainText -Force (cat .\Tools\mail_plain.txt)[1]))
}
if (-not $cred) {
    'メールサーバの資格情報を"Tools\mail_credential.ps1"で作成してください。'
    sleep 15
    exit
}
$M_TO = $cred.UserName
$M_FROM = $M_TO
$M_SUBJECT = 'EdcbPostChgReserve'
$M_BODY = "追従 $env:ServiceName $((Get-Date $env:StartTimeOLD).ToString('M/d H:mm'))～$((Get-Date $env:StartTimeOLD).AddSeconds($env:DurationSecondOLD).ToString('H:mm'))" `
        + " → $((Get-Date $env:StartTime).ToString('M/d H:mm'))～$((Get-Date $env:StartTime).AddSeconds($env:DurationSecond).ToString('H:mm')) $env:Title"

if ($env:EID16 -ne 'FFFF' -and ($env:StartTimeOLD -ne $env:StartTime -or $env:DurationSecondOLD -ne $env:DurationSecond)) {
    Send-MailMessage -SmtpServer $M_SMTP -Port $M_PORT -UseSsl -Credential $cred -To $M_TO -From $M_FROM -Subject $M_SUBJECT -Body $M_BODY -Encoding UTF8
}
