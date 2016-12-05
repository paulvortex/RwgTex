@echo off
set rwgtex=..\win32\rwgtex.exe
IF "%1"=="" (
	set profile=regular
) ELSE (
	set profile=%1
	set pausable=1
)
rmdir ~comparison_%profile% /S /Q
mkdir ~comparison_%profile%
echo --- running comparison (%profile%) ---
FOR %%i IN (images\*.*) DO (
	echo %%~ni
	REM --- DXT ---
	FOR %%t IN (nv ati gimp crunch pvrtex) DO (
		echo ..%%t ^(DXT^) : -dxt -%%t
		%rwgtex% -f "%%i" -dxt -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
		FOR %%x IN (rxgb ycg1 ycg2) DO (
			echo ..%%t ^(DXT,%%x^) : -dxt -%%t -%%x
			%rwgtex% -f "%%i" -dxt -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -%%x
		)
	)
	REM --- ETC1 ---
	FOR %%t IN (etcpack ati rgetc1 pvrtex) DO (
		echo ..%%t ^(ETC1^) : -etc1 -%%t
		%rwgtex% -f "%%i" -etc1 -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
	)
	REM --- ETC2 ---
	FOR %%t IN (etcpack, etc2comp pvrtex) DO (
		echo ..%%t ^(ETC2^) : -etc2 -%%t
		%rwgtex% -f "%%i" -etc2 -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
	)
	REM --- PVRTC ---
	FOR %%t IN (pvrtex) DO (
		FOR %%x IN (pvr2 pvr4) DO (
			echo ..%%t ^(PVRTC,%%x^) : -pvrtc -%%t -%%x
			%rwgtex% -f "%%i" -pvrtc -o "~comparison_%profile%\%%~ni" -%%t -%profile% -tk -%%x
		)
	)
)
IF "%pausable%"=="" (
	GOTO EXIT
)
pause
:EXIT
