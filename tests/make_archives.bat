@echo off
rmdir ~archives /S /Q
mkdir ~archives
set rwgtex=..\win32\rwgtex.exe
echo --- packing test.pk3 ---
%rwgtex% "images" -dxt -etc -pvr -o "~archives/test.pk3" -ap "textures/"
echo --- packing test.pk3 (zipmem) ---
%rwgtex% "images" -dxt -etc -pvr -o "~archives/test_zipmem.pk3" -ap "textures/" -zipmem 50
pause