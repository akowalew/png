#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#include "png.c"

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, int CmdShow)
{
    png Png;
    if(!PngLoad(CmdLine, &Png))
    {
        MessageBoxA(0, "Failed to load PNG", "The file is probably corrupted", MB_OK|MB_ICONERROR);
        return -1;
    }

    return 0;
}
