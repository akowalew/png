@echo off
set WDFLAGS=/wd4100 /wd4189 /wd4101
cl /nologo /Zi /Oi /W4 /WX %WDFLAGS% test.c /Fe:test.exe /link /DEBUG:FULL
test.exe
echo done