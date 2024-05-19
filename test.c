#include "png.c"

#include <assert.h>

typedef struct
{
    usz Sz;
    u8* At;
} buf;

typedef struct
{
    u32 Cols;
    u32 Rows;
    u32 Jump;
    u32 Size;
    u8* Data;
} bmp;

static b32 ReadEntireFile(const char* Path, buf* Buf)
{
    b32 Result = 0;

    HANDLE Handle = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(Handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(Handle, &FileSize))
        {
            if(!FileSize.HighPart)
            {
                void* At = VirtualAlloc(0, FileSize.LowPart, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                if(At)
                {
                    if(ReadFile(Handle, At, FileSize.LowPart, 0, 0))
                    {
                        Buf->Sz = FileSize.LowPart;
                        Buf->At = At;

                        Result = 1;
                    }
                    else
                    {
                        VirtualFree(At, 0, MEM_DECOMMIT|MEM_RELEASE);
                    }
                }
            }
        }

        CloseHandle(Handle);
    }

    return Result;
}

#pragma pack(push, 1)
typedef struct
{
    u8  Magic[2];
    u32 FileSize;
    u32 Reserved;
    u32 DataOffset;
} bmp_file_header;

typedef struct
{
    u32 Size;
    u32 Width;
    u32 Height;
    u16 NumPlanes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 ImageSize;
    u32 HorzRes;
    u32 VertRes;
    u32 PaletteColors;
    u32 ColorsImportant;
} bmp_info_header;
#pragma pack(pop)

static void* PopBytes(buf* Buf, usz Cnt)
{
    void* Result = 0;

    if(Buf->Sz >= Cnt)
    {
        Buf->Sz -= Cnt;
        Result = Buf->At;
        Buf->At += Cnt;
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}

#define PopType(Buf, x) (x*)PopBytes((Buf), sizeof(x))

static b32 BmpRead(buf* Buf, bmp* Bmp)
{
    b32 Result = 0;

    usz Sz = Buf->Sz;
    u8* At = Buf->At;
    bmp_file_header* Fhdr = PopType(Buf, bmp_file_header);
    if(Fhdr && Fhdr->Magic[0] == 'B' && Fhdr->Magic[1] == 'M')
    {
        bmp_info_header* Ihdr = PopType(Buf, bmp_info_header);
        if(Ihdr && Ihdr->Size >= sizeof(*Ihdr))
        {
            if(Ihdr->NumPlanes == 1 &&
               Ihdr->BitsPerPixel == 32 &&
               Ihdr->Compression == 3)
            {
                u32 Jump = Ihdr->Width * 4;
                u32 Size = Ihdr->Height * Jump;
                if(Ihdr->ImageSize == Size &&
                   Fhdr->DataOffset + Size <= Sz)
                {
                    Bmp->Cols = Ihdr->Width;
                    Bmp->Rows = Ihdr->Height;
                    Bmp->Jump = Jump;
                    Bmp->Size = Size;
                    Bmp->Data = &At[Fhdr->DataOffset];

                    Result = 1;
                }
            }
        }
    }

    return Result;
}

static b32 BmpLoad(const char* Path, bmp* Bmp)
{
    b32 Result = 0;

    buf Buf;
    if(ReadEntireFile(Path, &Buf))
    {
        if(BmpRead(&Buf, Bmp))
        {
            Result = 1;
        }
    }

    return Result;
}



#define Assert(x) if(!(x)) { if(IsDebuggerPresent()) { __debugbreak(); } fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #x, __FILE__, __LINE__); exit(-1); }

int main(int Argc, char** Argv)
{
    bmp Bmp;
    png Png;

    Assert(BmpLoad("test.bmp", &Bmp));
    Assert(PngLoad("test.png", &Png));
    Assert(Bmp.Cols == Png.Cols);
    Assert(Bmp.Rows == Png.Rows);
    u8 *BmpData = &Bmp.Data[Bmp.Size - Bmp.Jump];
    u8 *PngData = Png.Data;
    for(u32 Row = 0; Row < Png.Rows; Row++)
    {
        u8* BmpAt = (u8*) BmpData;
        u8* PngAt = (u8*) PngData;

        for(u32 Col = 0; Col < Png.Cols; Col++)
        {
            // TODO: SIMD

            u8 PngR = PngAt[0], PngG = PngAt[1], PngB = PngAt[2], PngA = PngAt[3];
            u8 BmpB = BmpAt[0], BmpG = BmpAt[1], BmpR = BmpAt[2], BmpA = BmpAt[3];

            if(PngR != BmpR ||
               PngG != BmpG ||
               PngB != BmpB ||
               PngA != BmpA)
            {
                Assert(0);
            }

            BmpAt += 4;
            PngAt += 4;
        }

        BmpData -= Bmp.Jump;
        PngData += Png.Jump;
    }
}