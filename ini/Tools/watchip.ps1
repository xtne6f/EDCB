# グローバルIPを3分ごとにしらべて変化したらメールする常駐スクリプト

# Gmailをひな型にしたがなんでもいい
$M_SMTP = 'smtp.gmail.com'
$M_PORT = 587
if (Test-Path .\mail_credential.txt) {
    $cred = New-Object Management.Automation.PSCredential ((cat .\mail_credential.txt)[0], (ConvertTo-SecureString (cat .\mail_credential.txt)[1]))
} else {
    $cred = New-Object Management.Automation.PSCredential ((cat .\mail_plain.txt)[0], (ConvertTo-SecureString -AsPlainText -Force (cat .\mail_plain.txt)[1]))
}
if (-not $cred) {
    'メールサーバの資格情報を"mail_credential.ps1"で作成してください。'
    sleep 15
    exit
}
$M_TO = $cred.UserName
$M_FROM = $M_TO
$M_SUBJECT = 'watchip'
$M_BODY = 'https://{IP}:5510/'

$lastip = ''
$currip = ''
'Start watching your external IPv4 address.'
while (1) {
    # グローバルIPを抽出できるサイトならなんでもいい
    $currip = (Invoke-WebRequest -UseBasicParsing 'http://checkip.dyndns.org').Content -replace '[^0-9.]'
    if ($currip -ne $lastip) {
        "Sending SmtpServer=$M_SMTP Port=$M_PORT To=$M_TO CurrIP=$currip ..."
        Send-MailMessage -SmtpServer $M_SMTP -Port $M_PORT -UseSsl -Credential $cred -To $M_TO -From $M_FROM -Subject $M_SUBJECT -Body $M_BODY.Replace('{IP}', $currip) -Encoding UTF8
        'Done.'
    }
    $lastip = $currip
    sleep 180
}
