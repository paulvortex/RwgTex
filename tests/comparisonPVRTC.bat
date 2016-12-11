@echo off
set rwgtex=..\win32\rwgtex.exe
IF "%1"=="" (
	set profile=regular
) ELSE (
	set profile=%1
	set pausable=1
)
rmdir ~comparison_ETC_%profile% /S /Q
mkdir ~comparison_ETC_%profile%
echo --- running comparison (%profile%) ---
FOR %%i IN (images\*.*) DO (
	echo %%~ni
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
