cd /d "%~dp0"
set DOTNET_CLI_TELEMETRY_OPTOUT=1
dotnet publish EpgTimer.core.sln -p:Configuration=Release -p:Platform=x64 -p:PlatformTarget=x64 -p:PublishProfile=win-x64
@pause
