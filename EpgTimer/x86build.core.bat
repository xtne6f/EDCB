cd /d "%~dp0"
set DOTNET_CLI_TELEMETRY_OPTOUT=1
dotnet publish EpgTimer.core.sln -p:Configuration=Release -p:Platform=x86 -p:PlatformTarget=x86 -p:PublishProfile=win-x86
@pause
