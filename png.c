typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef uint32_t b32;
typedef size_t   usz;

typedef struct
{
    u32 Cols;
    u32 Rows;
    u64 Jump;
    u64 Size;
    u8* Data;
} png;

typedef struct
{
    u8* At;
    u64 Sz;
    u64 Of;
} png_buf;

static void* PngAlloc(u64 Size)
{
    void* Result = VirtualAlloc(0, Size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    return Result;
}

static void PngFree(void* Data)
{
    VirtualFree(Data, 0, MEM_DECOMMIT|MEM_RELEASE);
}

static b32 PngLoadFile(const char* Path, png_buf* Buf)
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
                void* At = PngAlloc(FileSize.LowPart);
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
                        PngFree(At);
                    }
                }
            }
        }

        CloseHandle(Handle);
    }

    return Result;
}

static void* PngPop(png_buf* Buf, u64 Count)
{
    void* Result = 0;

    if(Buf->Sz >= Count)
    {
        Result = Buf->At;

        Buf->Sz -= Count;
        Buf->At += Count;
    }

    return Result;
}

#define PNG_BSWP32(x) _byteswap_ulong(x)
#define PNG_BSWP16(x) _byteswap_ushort(x)

static b32 PngPopU32(png_buf* Buf, u32* Val)
{
    b32 Result = 0;

    u32* At = PngPop(Buf, sizeof(u32));
    if(At)
    {
        *Val = PNG_BSWP32(*At);

        Result = 1;
    }

    return Result;
}

typedef struct
{
    u32 Len;
    u32 Typ;
    u8* Dat;
} png_chunk;

#define PNG_TYP(a,b,c,d) ((a << 0) | (b << 8) | (c << 16) | (d << 24))

static const u32 PngCrcTable[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

u32 PngCrc(u8* At, u32 Sz)
{
    u32 Result = 0xFFFFFFFF;

    for(u8* End = At + Sz; At != End; At++)
    {
        Result = PngCrcTable[(Result ^ *At) & 0xff] ^ (Result >> 8);
    }

    return Result ^ 0xFFFFFFFF;
}

static void PngDebugPrintf(const char* Fmt, ...)
{
    char Buffer[4096];

    va_list Args;
    va_start(Args, Fmt);
    vsnprintf(Buffer, sizeof(Buffer), Fmt, Args);
    va_end(Args);

    OutputDebugStringA(Buffer);
}

#define PngDebugBreak() __debugbreak()

#define PngError(Fmt, ...) do { PngDebugPrintf("ERROR: " Fmt, __VA_ARGS__); PngDebugBreak(); } while(0)

static b32 PngPopChunk(png_buf* Buf, png_chunk* Chunk)
{
    b32 Result = 0;

    u32 Len;
    if(PngPopU32(Buf, &Len))
    {
        u32 Typ, Crc;
        u32 Off = Len + sizeof(Typ);
        u8* Mem = PngPop(Buf, Off + sizeof(Crc));
        if(Mem)
        {
            Typ = *(u32*)(&Mem[0]);
            Crc = *(u32*)(&Mem[Off]);
            u32 CrcReal = PNG_BSWP32(PngCrc(Mem, Off));
            if(Crc == CrcReal)
            {
                PngDebugPrintf("Chunk %c%c%c%c of length %10ld bytes\n", Mem[0], Mem[1], Mem[2], Mem[3], Len);
                Chunk->Len = Len;
                Chunk->Typ = Typ;
                Chunk->Dat = &Mem[sizeof(Typ)];
                Result = 1;
            }
        }
    }

    return Result;
}

#pragma pack(push, 1)
typedef struct
{
   u32 Cols;
   u32 Rows;
   u8  BitDepth;
   u8  ColorType;
   u8  CompressionMethod;
   u8  FilterMethod;
   u8  InterlaceMethod;
} png_ihdr;
#pragma pack(pop)

#define PNG_CTYPE_PALET 0x1
#define PNG_CTYPE_COLOR 0x2
#define PNG_CTYPE_ALPHA 0x4

static const u8 PNG_HEADER[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

#define PNG_CM_DEFLATE 8
#define PNG_FLG_FDICT (1 << 5)

#define PngAssert(x) if(!(x)) { *(int*)(0) = 0; }

#define PNG_COMP_NONE            0
#define PNG_COMP_FIXED_HUFFMAN   1
#define PNG_COMP_DYNAMIC_HUFFMAN 2

typedef struct
{
    u8* At;
    usz Sz;
    u64 Vl;
    u8  Nb;
} png_bstr;

static void PngRefill(png_bstr* Bstr)
{
    u8 Count = (64 - Bstr->Nb) / 8;
    while(Count--)
    {
        // TODO: Optimize it to do less fetches

        if(!Bstr->Sz)
        {
            PngDebugPrintf("Out of buffer\n");
            break;
        }

        u64 X = *(Bstr->At++);
        Bstr->Sz--;
        Bstr->Vl |= (X << Bstr->Nb);
        Bstr->Nb += 8;
    }
}

static u64 PngPopBits(png_bstr* Bstr, u8 Count)
{
    u64 Result;

    PngAssert(Count <= 64);
    PngAssert(Bstr->Nb >= Count);

    Result = Bstr->Vl & ((1 << Count) - 1);
    Bstr->Vl >>= Count;
    Bstr->Nb -= Count;

    return Result;
}

#define PNG_REVBITS8(x) ((u8)((((x) * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32))
#define PNG_REVBITS16(x) ((u16)((PNG_REVBITS8((x) & 0xFF) << 8) | PNG_REVBITS8((x) >> 8)))

#define PNG_PARSE_FAILURE 0
#define PNG_PARSE_NEXT_BLOCK 1
#define PNG_PARSE_FINAL_BLOCK 2

typedef struct
{
    u8 Codes[19];
    u8 Lengths[19];
} png_clht;

static const u8 PngCodeLengthMap[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static b32 PngParseCodeLengthsHuffmanTable(png_bstr* Bstr, u8 Total, u8* Lengths, u8* Codes)
{
    PngRefill(Bstr);
    if(Bstr->Nb < Total * 3)
    {
        PngError("Failed to read code lengths\n");
        return 0;
    }

    u8 Length;
    u64 Idx = 0;
    u8 Counts[8] = {0};
    while(Idx < Total)
    {
        Length = (u8) PngPopBits(Bstr, 3);
        Lengths[PngCodeLengthMap[Idx++]] = Length;
        Counts[Length]++;
    }
    while(Idx < 19)
    {
        Lengths[PngCodeLengthMap[Idx++]] = 0;
    }

    u8 Code = 0;
    Counts[0] = 0;
    u8 NextCodes[8];
    for(Idx = 1; Idx < 8; Idx++)
    {
        // TODO: To be honest we can clamp this loop just to MAX_BITS
        // But then we need to min/max this MAX_BITS... worth it?
        Code = (Code + Counts[Idx-1]) << 1;
        NextCodes[Idx] = Code;
    }

    for(Idx = 0; Idx < 19; Idx++)
    {
        Length = Lengths[Idx];
        Code = NextCodes[Length]++;
        Codes[Idx] = PNG_REVBITS8(Code) >> (8 - Length);
    }

    return 1;
}

#define PNG_NCODES (286 + 32 + 138)

typedef struct
{
    u16 LitCount;
    u8  DstCount;
    u8  Padding; // IMPORTANT!
    u8  Lengths[PNG_NCODES];
    u16 Codes[PNG_NCODES];
} png_hts;

static u8 PngDecodeLength(png_bstr* Bstr, u8* Lengths, u8* Codes)
{
    u8 Result = 0;
    while(Result < 19)
    {
        u8 Len = Lengths[Result];
        if(Len)
        {
            // TODO: SLOW
            u8 Code = Codes[Result];
            u8 Mask = ((1 << Len) - 1);
            if((Bstr->Vl & Mask) == Code)
            {
                Bstr->Nb -= Len;
                Bstr->Vl >>= Len;
                break;
            }
        }

        Result++;
    }

    return Result;
}

static b32 PngParseLengths(png_bstr* Bstr, png_clht* ClHt, u8* Lengths, u32 Total)
{
    u32 Idx = 0;
    Lengths[-1] = 0;
    while(Idx < Total)
    {
        PngRefill(Bstr); // TODO: Can we reduce the number of refillings?
        if(Bstr->Nb < 14)
        {
            PngError("Failed to read next symbol\n");
            return 0;
        }

        u8 Length = PngDecodeLength(Bstr, ClHt->Lengths, ClHt->Codes);
        if(Length < 16)
        {
            Lengths[Idx++] = Length;
        }
        else if(Length == 16)
        {
            u8 Previous = Lengths[Idx-1];
            u8 Count = (u8) PngPopBits(Bstr, 2) + 3;
            while(Count--)
            {
                Lengths[Idx++] = Previous;
            }
        }
        else if(Length == 17)
        {
            u64 Count = PngPopBits(Bstr, 3) + 3;
            while(Count--)
            {
                Lengths[Idx++] = 0;
            }
        }
        else if(Length == 18)
        {
            u64 Count = PngPopBits(Bstr, 7) + 11;
            while(Count--)
            {
                Lengths[Idx++] = 0;
            }
        }
        else
        {
            PngError("Failed to decode length\n");
            return 0;
        }
    }

    if(Idx != Total)
    {
        PngError("Failed to parse lengths\n");
        return 0;
    }

    return 1;
}

static void PngBuildTable(png_bstr* Bstr, u8* Lengths, u16* Codes, u32 Total)
{
    u32 Idx;
    u8 Length;
    u16 Counts[16] = {0};
    for(Idx = 0; Idx < Total; Idx++)
    {
        Length = Lengths[Idx];
        Counts[Length]++;
    }

    u16 Code = 0;
    Counts[0] = 0;
    u16 NextCodes[16];
    for(Idx = 1; Idx < 16; Idx++)
    {
        Code = (Code + Counts[Idx-1]) << 1;
        NextCodes[Idx] = Code;
    }

    for(Idx = 0; Idx < Total; Idx++)
    {
        Length = Lengths[Idx];
        Code = NextCodes[Length]++;
        Codes[Idx] = PNG_REVBITS16(Code) >> (16 - Length);
    }
}

static b32 PngParseDynamicHuffmanTables(png_bstr* Bstr, png_hts* Hts)
{
    if(Bstr->Nb < 5 + 5 + 4)
    {
        PngError("Failed to read dynamic huffman table header\n");
        return 0;
    }

    u16 LitCount = (u16) PngPopBits(Bstr, 5) + 257;
    u8  DstCount = (u8)  PngPopBits(Bstr, 5) + 1;
    u8  ClCount  = (u8)  PngPopBits(Bstr, 4) + 4;

    png_clht ClHt;
    if(!PngParseCodeLengthsHuffmanTable(Bstr, ClCount, ClHt.Lengths, ClHt.Codes))
    {
        PngError("Failed to parse code lengths huffman table\n");
        return 0;
    }

    u32 Total = LitCount + DstCount;
    if(!PngParseLengths(Bstr, &ClHt, Hts->Lengths, Total))
    {
        PngError("Failed to parse lengths\n");
        return 0;
    }

    PngBuildTable(Bstr, &Hts->Lengths[0], &Hts->Codes[0], LitCount);
    PngBuildTable(Bstr, &Hts->Lengths[LitCount], &Hts->Codes[LitCount], DstCount);
    Hts->LitCount = LitCount;
    Hts->DstCount = DstCount;

    return 1;
}

static u16 PngDecodeSymbol(png_bstr* Bstr, const u8* Lengths, const u16* Codes, u16 Total)
{
    u16 Result = 0;
    while(Result < Total)
    {
        u8 Len = Lengths[Result];
        if(Len)
        {
            // TODO: SLOW
            u16 Code = Codes[Result];
            u16 Mask = ((1 << Len) - 1);
            if((Bstr->Vl & Mask) == Code)
            {
                Bstr->Nb -= Len;
                Bstr->Vl >>= Len;
                break;
            }
        }

        Result++;
    }

    return Result;
}

static b32 PngParseDeflateBlock(png_bstr* Bstr, png_buf* Img)
{
    PngRefill(Bstr);
    if(Bstr->Nb < 3)
    {
        PngError("Failed to read deflate block header\n");
        return PNG_PARSE_FAILURE;
    }

    u8 BFINAL = (u8) PngPopBits(Bstr, 1);
    u8 BTYPE = (u8) PngPopBits(Bstr, 2);
    if(BTYPE == PNG_COMP_NONE)
    {
        PngAssert(0); // Not implemented yet
    }
    else
    {
        static const png_hts FixedHuffman = {0};
        png_hts DynamicHuffman;

        const png_hts* Hts;
        if(BTYPE == PNG_COMP_DYNAMIC_HUFFMAN)
        {
            if(!PngParseDynamicHuffmanTables(Bstr, &DynamicHuffman))
            {
                PngError("Failed to parse dynamic huffman table\n");
                return PNG_PARSE_FAILURE;
            }

            Hts = &DynamicHuffman;
        }
        else
        {
            PngAssert(0);

            Hts = &FixedHuffman;
        }

        while(1)
        {
            PngRefill(Bstr);
            if(Bstr->Nb < (15 + 5) + (15 + 13))
            {
                PngError("Out of buffer\n");
                return PNG_PARSE_FAILURE;
            }

            u16 LitLen = PngDecodeSymbol(Bstr, Hts->Lengths, Hts->Codes, Hts->LitCount);
            if(LitLen < 256)
            {
                if(Img->Of >= Img->Sz)
                {
                    PngError("Out of image\n");
                    return PNG_PARSE_FAILURE;
                }

                Img->At[Img->Of++] = (u8) LitLen;
            }
            else if(LitLen == 256)
            {
                break; // End of block
            }
            else if(LitLen >= 257 && LitLen < Hts->LitCount)
            {
                static const u16 LengthsBases[] =
                {
                    3, 4, 5, 6, 7, 8, 9, 10, // 257 - 264
                    11, 13, 15, 17,          // 265 - 268
                    19, 23, 27, 31,          // 269 - 273
                    35, 43, 51, 59,          // 274 - 276
                    67, 83, 99, 115,         // 278 - 280
                    131, 163, 195, 227,      // 281 - 284
                    258                      // 285
                };

                static const u8 LengthsExtraBits[] =
                {
                    0, 0, 0, 0, 0, 0, 0, 0, // 257 - 264
                    1, 1, 1, 1,             // 265 - 268
                    2, 2, 2, 2,             // 269 - 273
                    3, 3, 3, 3,             // 274 - 276
                    4, 4, 4, 4,             // 278 - 280
                    5, 5, 5, 5,             // 281 - 284
                    0                       // 285
                };

                u8 LengthOffset = (u8) (LitLen - 257);
                u16 LengthExtra = (u16) PngPopBits(Bstr, LengthsExtraBits[LengthOffset]);
                u16 LengthTotal = LengthExtra + LengthsBases[LengthOffset];

                u16 DistanceOffset = PngDecodeSymbol(Bstr, &Hts->Lengths[Hts->LitCount], &Hts->Codes[Hts->LitCount], Hts->DstCount);
                if(DistanceOffset > 29)
                {
                    PngError("Failed to decode distance\n");
                    return PNG_PARSE_FAILURE;
                }

                static const u32 DistancesBases[] =
                {
                    1, 2, 3, 4,   // 0 - 3
                    5, 7,         // 4 - 5
                    9, 13,        // 6 - 7
                    17, 25,       // 8 - 9
                    33, 49,       // 10 - 11
                    65, 97,       // 12 - 13
                    129, 193,     // 14 - 15
                    257, 385,     // 16 - 17
                    513, 769,     // 18 - 19
                    1025, 1537,   // 20 - 21
                    2049, 3073,   // 22 - 23
                    4097, 6145,   // 24 - 25
                    8193, 12289,  // 26 - 27
                    16385, 24577  // 28 - 29
                };

                static const u8 DistancesExtraBits[] =
                {
                    0, 0, 0, 0, // 0 - 3
                    1, 1,       // 4 - 5
                    2, 2,       // 6 - 7
                    3, 3,       // 8 - 9
                    4, 4,       // 10 - 11
                    5, 5,       // 12 - 13
                    6, 6,       // 14 - 15
                    7, 7,       // 16 - 17
                    8, 8,       // 18 - 19
                    9, 9,       // 20 - 21
                    10, 10,     // 22 - 23
                    11, 11,     // 24 - 25
                    12, 12,     // 26 - 27
                    13, 13      // 28 - 29
                };

                u16 DistanceExtra = (u16) PngPopBits(Bstr, DistancesExtraBits[DistanceOffset]);
                u32 DistanceTotal = DistanceExtra + DistancesBases[DistanceOffset];
                i64 Position = Img->Of - DistanceTotal;
                if(Position < 0)
                {
                    PngError("Invalid back distance decoded\n");
                    return PNG_PARSE_FAILURE;
                }

                if(Img->Of + LengthTotal > Img->Sz)
                {
                    PngError("Out of image\n");
                    return PNG_PARSE_FAILURE;
                }

                // TODO: Optimize this!!!
                for(u64 Idx = 0; Idx < LengthTotal; Idx++)
                {
                    Img->At[Img->Of+Idx] = Img->At[Position+Idx];
                }

                Img->Of += LengthTotal;
            }
            else
            {
                PngError("Literal symbol decode failed\n");
                return PNG_PARSE_FAILURE;
            }
        }
    }

    return BFINAL ? PNG_PARSE_FINAL_BLOCK : PNG_PARSE_NEXT_BLOCK;
}

static b32 PngParseIdat(u8* Dat, u64 Len, png_buf* Img)
{
    if(Len <= 2)
    {
        PngError("Invalid IDAT chunk length: %d\n", Len);
        return 0;
    }

    u8 CMF = Dat[0];
    u8 FLG = Dat[1];
    u8 CM    = CMF & 0x0f;
    u8 CINFO = CMF >> 4;
    if((CM != PNG_CM_DEFLATE) || (FLG & PNG_FLG_FDICT)  || ((CMF*256 + FLG) % 31))
    {
        PngError("Invalid CM and/or FLG for IDAT: 0x%X 0x%02X\n", CM, FLG);
        return 0;
    }

    png_bstr Bstr;
    Bstr.At = &Dat[2];
    Bstr.Sz = Len + 8; // IMPORTANT: Leave 8 byte space at the end to reduce number of if-s
    Bstr.Nb = 0;
    Bstr.Vl = 0;

    b32 Result;
    while((Result = PngParseDeflateBlock(&Bstr, Img)) != PNG_PARSE_FAILURE)
    {
        if(Result == PNG_PARSE_FINAL_BLOCK)
        {
            break;
        }
    }

    return Result;
}

#define PNG_FILTER_NONE    0
#define PNG_FILTER_SUB     1
#define PNG_FILTER_UP      2
#define PNG_FILTER_AVERAGE 3
#define PNG_FILTER_PAETH   4

static inline u8 PngPaeth(u8 A, u8 B, u8 C)
{
    // TODO: Optimize this!!!
    int PA = abs(B - C);
    int PB = abs(A - C);
    int PC = abs(A + B - 2 * C);
    if(PA <= PB && PA <= PC)
    {
        return A;
    }
    else if(PB <= PC)
    {
        return B;
    }
    else
    {
        return C;
    }
}

static b32 PngParse(png_buf* Buf, png* Png)
{
    u8* Header = PngPop(Buf, sizeof(PNG_HEADER));
    if(!Header && memcmp(Header, PNG_HEADER, sizeof(PNG_HEADER)))
    {
        PngError("Invalid or missing PNG header\n");
        return 0;
    }

    png_chunk Chunk;
    if(!PngPopChunk(Buf, &Chunk) &&
       Chunk.Typ != PNG_TYP('I','H','D','R') &&
       Chunk.Len < sizeof(png_ihdr))
    {
        PngError("Invalid or missing IHDR chunk\n");
        return 0;
    }

    png_ihdr* IHDR = (png_ihdr*) Chunk.Dat;
    Png->Cols = PNG_BSWP32(IHDR->Cols);
    Png->Rows = PNG_BSWP32(IHDR->Rows);

    u32 Nbpp = 0;
    u64 Nbpr = 0;
    u64 RxdJump = 0;
    u64 RxdSize = 0;
    if(IHDR->CompressionMethod == 0 &&
       IHDR->FilterMethod == 0 &&
       IHDR->InterlaceMethod == 0)
    {
        // TODO: Eventually we should move all this header bytes into `png` struct

        if(IHDR->BitDepth == 8)
        {
            if(IHDR->ColorType == (PNG_CTYPE_COLOR|PNG_CTYPE_ALPHA))
            {
                Nbpp = 4;
                Nbpr = Png->Cols * Nbpp;

                RxdJump = 1 + Nbpr; // One extra byte for FILTER byte in begin of each line
                RxdSize = RxdJump * Png->Rows;

                Png->Jump = Nbpp + Nbpr; // One extra column in begin of each line for SUB filter
                Png->Size = Png->Jump * (Png->Rows + 1); // One extra row in begin for UP filter
            }
            else
            {
                PngError("Unsupported PNG color type: 0x%02X\n", IHDR->ColorType);
                return 0;
            }
        }
        else
        {
            PngError("Unsupported PNG bit depth: %d\n", IHDR->BitDepth);
            return 0;
        }
    }
    else
    {
        PngError("Unsupported CompressionMethod/FilterMethod/InterlaceMethod %d/%d/%d\n",
                       IHDR->CompressionMethod, IHDR->FilterMethod, IHDR->InterlaceMethod);
        return 0;
    }

    // TODO: Maybe better is to use two allocations, so that the final image
    // will not begin at the page boundary, which will improve cache coherency?
    u64 Size = RxdSize + Png->Size;
    // IMPORTANT: We assume that pages are zeroed!!!!!!
    u8* Data = PngAlloc(Size);
    if(!Data)
    {
        PngError("Failed to allocate memory for PNG image\n");
        return 0;
    }

    u8* RxdData = &Data[0];

    png_buf Rxd;
    Rxd.At = RxdData;
    Rxd.Sz = RxdSize;
    Rxd.Of = 0;

    b32 Result = 0;
    b32 Continue = 1;
    while(Continue && PngPopChunk(Buf, &Chunk))
    {
        switch(Chunk.Typ)
        {
            case PNG_TYP('g','A','M','A'):
            {
                PngError("Gamma correction not supported yet\n");
            } break;

            case PNG_TYP('I','D','A','T'):
            {
                if(Buf->Sz < 4)
                {
                    // IMPORTANT: Chunk ends with 32b CRC, and then MUST BE next the IEND chunk, so
                    // there should be at least 8 bytes more in buffer than chunk needs. This check
                    // helps to assume certain things in further parsing and reduces if-s statements.
                    PngError("Invalid PNG file\n");
                    Continue = 0;
                }

                if(!PngParseIdat(Chunk.Dat, Chunk.Len, &Rxd))
                {
                    PngError("PNG IDAT chunk invalid");
                    Continue = 0;
                }
            } break;

            case PNG_TYP('I','E','N','D'):
            {
                Continue = 0;
                Result = 1;
            } break;
        }
    }

    if(!Result)
    {
        PngFree(Data);
        return 0;
    }

    // Skip left margin for SUB filter and first column for UP filter
    Png->Data = &Data[RxdSize + Png->Jump + Nbpp];

    u8* RxdLine = RxdData;
    u8* PngLine = Png->Data;
    for(u64 Row = 0; Row < Png->Rows; Row++)
    {
        u8* RxdAt = RxdLine;
        u8* PngAt = PngLine;

        u8 Filter = *(RxdAt++);
        switch(Filter)
        {
            case PNG_FILTER_SUB:
            {
                for(u64 Idx = 0; Idx < Nbpr; Idx++)
                {
                    // TODO: Optimize this!
                    *PngAt = *RxdAt + *(PngAt - Nbpp);
                    PngAt++; RxdAt++;
                }
            } break;

            case PNG_FILTER_UP:
            {
                for(u64 Idx = 0; Idx < Nbpr; Idx++)
                {
                    // TODO: Optimize this!
                    *PngAt = *RxdAt + *(PngAt - Png->Jump);
                    PngAt++; RxdAt++;
                }
            } break;

            case PNG_FILTER_AVERAGE:
            {
                for(u64 Idx = 0; Idx < Nbpr; Idx++)
                {
                    // TODO: Optimize this!
                    *PngAt = *RxdAt + (*(PngAt - Nbpp) + *(PngAt - Png->Jump)) / 2;
                    PngAt++; RxdAt++;
                }
            } break;

            case PNG_FILTER_PAETH:
            {
                for(u64 Idx = 0; Idx < Nbpr; Idx++)
                {
                    // TODO: Optimize this!
                    *PngAt = *RxdAt + PngPaeth(*(PngAt - Nbpp), *(PngAt - Png->Jump), *(PngAt - Png->Jump - Nbpp));
                    PngAt++; RxdAt++;
                }
            } break;

            default:
                PngDebugPrintf("Unrecognized filter type for line %d: 0x%02X\n", Row, Filter);
            case PNG_FILTER_NONE: // fallthrough
                memcpy(PngAt, RxdAt, Nbpr);
            break;
        }

        RxdLine += RxdJump;
        PngLine += Png->Jump;
    }

    return 1;
}

static b32 PngLoad(const char* Path, png* Png)
{
    b32 Result = 0;

    png_buf Buf;
    if(PngLoadFile(Path, &Buf))
    {
        void* At = Buf.At;

        if(PngParse(&Buf, Png))
        {
            Result = 1;
        }

        PngFree(At);
    }

    return Result;
}