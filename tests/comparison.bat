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
		%rwgtex% -f "%%i" -dxt -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
		echo ..%%t ^(DXT,sRGB^)
		%rwgtex% -f "%%i" -dxt -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -srgb
		FOR %%x IN (rxgb ycg1 ycg2) DO (
			echo ..%%t ^(DXT,%%x^)
			%rwgtex% -f "%%i" -dxt -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -%%x
			echo ..%%t ^(DXT,%%x,sRGB^)
			%rwgtex% -f "%%i" -dxt -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t -%%x -srgb
		)

	)
	REM --- ETC1 ---
	FOR %%t IN (etcpack ati rgetc1 pvrtex) DO (
		echo ..%%t ^(ETC1^)
		%rwgtex% -f "%%i" -etc1 -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
	)
	REM --- ETC2 ---
	FOR %%t IN (etcpack) DO (
		echo ..%%t ^(ETC2^)
		%rwgtex% -f "%%i" -etc2 -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
	)
	REM --- PVRTC (4BPP) ---
	FOR %%t IN (pvrtex) DO (
		echo ..%%t ^(PVRTC 4BPP^)
		%rwgtex% -f "%%i" -pvrtc -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
	)
	REM --- PVRTC (2BPP) ---
	FOR %%t IN (pvrtex) DO (
		echo ..%%t ^(PVRTC 2BPP^)
		%rwgtex% -f "%%i" -pvrtc2 -o "~comparison_%profile%\%%~ni" -%%t -%profile% -t
	)
	
)
IF "%pausable%"=="" (
	GOTO EXIT
)
pause
:EXIT
