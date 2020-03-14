set MINGW_ROOT=
@rem set MINGW_ROOT=%USERPROFILE%\mingw32
@rem set MINGW_ROOT=%USERPROFILE%\mingw64
@rem set MINGW_ROOT=%SystemDrive%\mingw32
@rem set MINGW_ROOT=%SystemDrive%\mingw64

set CPPFLAGS=
@rem set CPPFLAGS=-DARIB8CHAR_USE_UNICODE %CPPFLAGS%
@rem set CPPFLAGS=-DEPGDB_STD_WREGEX %CPPFLAGS%

@if exist "%MINGW_ROOT%\bin" goto label1
@echo Error: "%MINGW_ROOT%\bin" Not Found.
@pause
@goto label9

:label1
set PATH=%MINGW_ROOT%\bin;%PATH%
mingw32-make CC=gcc RM=del %*
@echo.
@echo Done(%errorlevel%).
@pause

:label9
