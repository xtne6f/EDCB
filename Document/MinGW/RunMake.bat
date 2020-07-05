set MINGW_ROOT=
@rem set MINGW_ROOT=%USERPROFILE%\mingw32
@rem set MINGW_ROOT=%USERPROFILE%\mingw64
@rem set MINGW_ROOT=%SystemDrive%\mingw32
@rem set MINGW_ROOT=%SystemDrive%\mingw64

@rem set CPPFLAGS=-DEPGDB_STD_WREGEX
@rem set LDFLAGS=-Wl,-Map=$(basename $(TARGET)).map

@if not "%MINGW_ROOT%"=="" goto label2
@echo.
@echo Searching for MinGW installed folder from "%USERPROFILE%" or "%SystemDrive%\"...
@set /p x="Select folder: 1=mingw32(for x86-build), 2=mingw64(for x64-build) : "
@if "%x%"=="1" goto label_x86
@if "%x%"=="2" goto label_x64
@goto label9

:label_x86
@set x=%USERPROFILE%\mingw32\mingw32
@if not exist "%x%" set x=%USERPROFILE%\mingw32
@if not exist "%x%" set x=%SystemDrive%\mingw32\mingw32
@if not exist "%x%" set x=%SystemDrive%\mingw32
set MINGW_ROOT=%x%
@goto label2

:label_x64
@set x=%USERPROFILE%\mingw64\mingw64
@if not exist "%x%" set x=%USERPROFILE%\mingw64
@if not exist "%x%" set x=%SystemDrive%\mingw64\mingw64
@if not exist "%x%" set x=%SystemDrive%\mingw64
set MINGW_ROOT=%x%

:label2
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
