cd /d "%~dp0"
%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe EpgTimer.sln /p:Configuration=Release /p:Platform=x86
@pause
