@echo off
set rwgtex=..\win32\rwgtex.exe
%rwgtex% -f "images/brick1_1_norm.tga" -etc -stf -pvrtex -regular -t -nocache
pause