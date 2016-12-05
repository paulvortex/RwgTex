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
	REM --- ETC1 ---
	FOR %%t IN (etcpack ati rgetc1 pvrtex) DO (
		echo ..%%t ^(ETC1^)
		%rwgtex% -f "%%i" -etc1 -o "~comparison_ETC_%profile%\%%~ni" -%%t -%profile% -t
	)
	REM --- ETC2 ---
	FOR %%t IN (etcpack, etc2comp pvrtex) DO (
		echo ..%%t ^(ETC2^)
		%rwgtex% -f "%%i" -etc2 -o "~comparison_ETC_%profile%\%%~ni" -%%t -%profile% -t
	)
)
IF "%pausable%"=="" (
	GOTO EXIT
)
pause
:EXIT
