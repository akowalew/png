#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#include "png.c"

#define PNAME "PngView"

static LRESULT WindowProc(HWND Hwnd, UINT Umsg, WPARAM Wpar, LPARAM Lpar)
{
    switch(Umsg)
    {
        case WM_KEYDOWN:
        {
            switch(Wpar)
            {
                case VK_ESCAPE:
                {
                    DestroyWindow(Hwnd);
                } break;
            }
        } break;

        case WM_CLOSE:
        {
            DestroyWindow(Hwnd);
        } break;
    }

    return DefWindowProc(Hwnd, Umsg, Wpar, Lpar);
}

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, int CmdShow)
{
    png Png;
    if(!PngLoad(CmdLine, &Png))
    {
        MessageBoxA(0, "The file is probably corrupted", PNAME, MB_OK|MB_ICONERROR);
        return -1;
    }

    WNDCLASSEX WindowClassEx = {0};
    WindowClassEx.cbSize = sizeof(WindowClassEx);
    WindowClassEx.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_DROPSHADOW;
    WindowClassEx.lpfnWndProc = WindowProc;
    WindowClassEx.cbClsExtra = 0;
    WindowClassEx.cbWndExtra = 0;
    WindowClassEx.hInstance = Instance;
    WindowClassEx.hIcon = 0;
    WindowClassEx.hCursor = 0;
    WindowClassEx.hbrBackground = 0;
    WindowClassEx.lpszMenuName = 0;
    WindowClassEx.lpszClassName = PNAME;
    WindowClassEx.hIconSm = 0;
    if(!RegisterClassEx(&WindowClassEx))
    {
        MessageBoxA(0, "Failed to register window class", PNAME, MB_OK|MB_ICONERROR);
        return -1;
    }

    HWND Hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
                               PNAME, PNAME,
                               WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               0, 0, Instance, 0);
    if(!Hwnd)
    {
        MessageBoxA(0, "Failed to create window", PNAME, MB_OK|MB_ICONERROR);
        return -1;
    }

    HDC Hdc = GetDC(Hwnd);
    if(!Hdc)
    {
        MessageBoxA(Hwnd, "Failed to get device context", PNAME, MB_OK|MB_ICONERROR);
        return -1;
    }

    MSG Msg;
    BOOL Result;
    while((Result = GetMessage(&Msg, Hwnd, 0, 0)) > 0)
    {
        if(Msg.message == WM_QUIT)
        {
            break;
        }

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);

        RECT ClientRect = {0};
        if(!GetClientRect(Hwnd, &ClientRect))
        {
            MessageBoxA(Hwnd, "Failed to get client rect", PNAME, MB_OK|MB_ICONERROR);
            return -1;
        }

        int ClientCols = ClientRect.right - ClientRect.left;
        int ClientRows = ClientRect.bottom - ClientRect.top;

        BITMAPINFO BitmapInfo = {0};
        BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
        BitmapInfo.bmiHeader.biWidth = Png.Cols + 1;
        BitmapInfo.bmiHeader.biHeight = -(int)(Png.Rows);
        BitmapInfo.bmiHeader.biPlanes = 1;
        BitmapInfo.bmiHeader.biBitCount = 32;
        BitmapInfo.bmiHeader.biCompression = BI_RGB;
        BitmapInfo.bmiHeader.biSizeImage = 0;
        BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
        BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
        BitmapInfo.bmiHeader.biClrUsed = 0;
        BitmapInfo.bmiHeader.biClrImportant = 0;

#if 0
        StretchDIBits(Hdc,
                      0, 0, ClientCols, ClientRows,
                      1, 0, Png.Cols + 1, Png.Rows, Png.Data,
                      &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
#else
        SetDIBitsToDevice(Hdc,
                          0, 0,
                          Png.Cols, Png.Rows,
                          1, 0, 0,
                          Png.Rows,
                          Png.Data,
                          &BitmapInfo,
                          DIB_RGB_COLORS);
#endif
    }

    return 0;
}
