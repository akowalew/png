@echo off
set WDFLAGS=/wd4100 /wd4189 /wd4101
cl /nologo /Zi /W4 /WX %WDFLAGS% win32_png_view.c /Fe:png_view.exe /link /DEBUG:FULL user32.lib gdi32.lib /SUBSYSTEM:WINDOWS
echo done