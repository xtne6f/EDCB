@echo This script converts *.h and *.cpp to Shift_JIS encoding.
@pause
cd /d "%~dp0"
powershell -NoProfile -Command "&{ls -Recurse -Include *.h,*.cpp -Exclude CommonResource.h,resource.h,targetver.h|foreach{$_.FullName;$r=New-Object IO.StreamReader($_,[Text.Encoding]::GetEncoding(932));$s=$r.ReadToEnd();$r.Close();$w=New-Object IO.StreamWriter($_,$false,[Text.Encoding]::GetEncoding(932));$w.Write($s);$w.Close()}}"
@echo Done.
@pause
