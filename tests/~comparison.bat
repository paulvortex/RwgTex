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
FOR %%i IN (images\*.tga) DO (
	echo %%~ni
	REM --- DXT ---
	FOR %%t IN (nv ati gimp crunch pvrtex) DO (
		echo ..%%t ^(DXT^)
		%rwgtex% -f "%%i" -dxt -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -nocache
	)
	REM --- ETC1 ---
	FOR %%t IN (etcpack ati rgetc1 pvrtex) DO (
		echo ..%%t ^(ETC1^)
		%rwgtex% -f "%%i" -etc -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -nocache
	)
	REM --- PVRTC (4BPP) ---
	FOR %%t IN (pvrtex) DO (
		echo ..%%t ^(PVRTC 4BPP^)
		%rwgtex% -f "%%i" -pvr -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -nocache
	)
	REM --- PVRTC (2BPP) ---
	FOR %%t IN (pvrtex) DO (
		echo ..%%t ^(PVRTC 2BPP^)
		%rwgtex% -f "%%i" -pvr -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -nocache -pvr2
	)
	
)
IF "%pausable%"=="" (
	GOTO EXIT
)
pause
:EXIT
