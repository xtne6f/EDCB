# 録画終了時にメールするスクリプト
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
$M_SUBJECT = 'EdcbPostRecEnd'
$M_BODY = "$env:Result $env:ServiceName $((Get-Date $env:StartTime).ToString('M/d H:mm'))～$((Get-Date $env:StartTime).AddSeconds($env:DurationSecond).ToString('H:mm')) D:$env:Drops S:$env:Scrambles $env:Title"

Send-MailMessage -SmtpServer $M_SMTP -Port $M_PORT -UseSsl -Credential $cred -To $M_TO -From $M_FROM -Subject $M_SUBJECT -Body $M_BODY -Encoding UTF8
