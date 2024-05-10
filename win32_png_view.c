#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <gL/gl.h>

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

static void Win32CheckFailed(const char* File, int Line, const char* Expr)
{
    char String[4096];
    DWORD LastError = GetLastError();
    size_t size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 LastError,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 String,
                                 sizeof(String)-1,
                                 NULL);

    MessageBoxA(0, String, PNAME, MB_OK|MB_ICONERROR);
}

#define Win32Check(x) if(!(x)) { Win32CheckFailed(__FILE__, __LINE__, #x); __debugbreak(); }

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
    Win32Check(RegisterClassEx(&WindowClassEx));

    DWORD Style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    DWORD ExStyle = WS_EX_OVERLAPPEDWINDOW;

    RECT WindowRect;
    WindowRect.left = 0;
    WindowRect.right = Png.Cols;
    WindowRect.top = 0;
    WindowRect.bottom = Png.Rows;
    Win32Check(AdjustWindowRectEx(&WindowRect, Style, 0, ExStyle));
    i32 WindowCols = WindowRect.right - WindowRect.left;
    i32 WindowRows = WindowRect.bottom - WindowRect.top;

    HWND Hwnd = CreateWindowEx(ExStyle,
                               PNAME, PNAME,
                               Style,
                               CW_USEDEFAULT, CW_USEDEFAULT,
                               WindowCols, WindowRows,
                               0, 0, Instance, 0);
    Win32Check(Hwnd);

    HDC Hdc = GetDC(Hwnd);
    Win32Check(Hdc);

    PIXELFORMATDESCRIPTOR PixelFormatDescriptor = {0};
    PixelFormatDescriptor.nSize = sizeof(PixelFormatDescriptor);
    PixelFormatDescriptor.nVersion = 1;
    PixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    PixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
    PixelFormatDescriptor.cColorBits = 32;
    PixelFormatDescriptor.cAlphaBits = 8;
    PixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE;

    int SuggestedPixelFormatIndex = ChoosePixelFormat(Hdc, &PixelFormatDescriptor);
    Win32Check(SuggestedPixelFormatIndex);
    Win32Check(DescribePixelFormat(Hdc, SuggestedPixelFormatIndex, sizeof(PixelFormatDescriptor), &PixelFormatDescriptor));
    Win32Check(SetPixelFormat(Hdc, SuggestedPixelFormatIndex, &PixelFormatDescriptor));

    HGLRC Hglrc = wglCreateContext(Hdc);
    Win32Check(Hglrc);
    Win32Check(wglMakeCurrent(Hdc, Hglrc));

    #define GL_BGRA                           0x80E1 // TODO: We should check for that extension

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    u32 Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, Png.Cols+1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Png.Cols, Png.Rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, Png.Data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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

        glViewport(0, 0, ClientCols, ClientRows);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glBegin(GL_TRIANGLES);
        {
            //
            // First Triangle
            //

            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(-1.0f, -1.0f);

            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-1.0f, +1.0f);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(+1.0f, -1.0f);

            //
            // Second Triangle
            //

            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(+1.0f, -1.0f);

            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(+1.0f, +1.0f);

            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-1.0f, +1.0f);
        }
        glEnd();

        SwapBuffers(Hdc);
    }

    return 0;
}
