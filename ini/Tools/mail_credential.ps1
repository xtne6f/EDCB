'メールサーバの資格情報を他ユーザに解読されない形で"mail_credential.txt"に保存します...'
'(EpgTimerSrvをサービスとして使用する場合はユーザ名とパスワードを"mail_plain.txt"に直接保存してください)'
$ErrorActionPreference = 'Stop'
$cred = Get-Credential 'メールアドレスなど'
"$($cred.UserName)`r`n$($cred.Password | ConvertFrom-SecureString)" | Set-Content -Encoding UTF8 .\mail_credential.txt
