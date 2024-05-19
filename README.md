# png

Free time fun hobby project - let's write a PNG parser.

Everybody is blaming about reinventing the wheel... but there are so many types of wheels! 

How can you learn it if you don't write it first? ;) I am kind of guy that is learning through exploring.

## Current state

![image](https://github.com/akowalew/png/assets/11333571/d0294130-7020-48b8-afda-a2c9c70e81cd)

1. Reads RGBA8888 images only at the moment - yay!!!
2. Single C file - just `png.c` with everything in
3. No dependencies and extremely fast to compile!
4. No interlace support at the moment
5. Only dynamic huffman table support at the moment (easy to add fixed one)
6. No uncompressed deflate blocks support at the moment (easy to add)
7. No SIMD used at the moment (but it really wants to!)
8. No multithreading support at the moment (but should it have?)
9. No gamma support at the moment (but easy to add especially with SIMD!)
10. No pallette support at the moment (does anybody care about that anymore?)
11. Very simple huffman tree traversal implemented - can be optimized!
12. Windows x64 only (adding more platforms is easy)
13. Test application for viewing PNGs - written for Win32 OpenGL backend only at the moment
14. All filters implemented - just need to optimize them
15. Only IHDR, IDAT, IEND chunks supported at the moment
16. MSVC only at the moment (adding more compilers is easy)

## Building

Surprise - just hit `build.bat` and you are in!

Then just run `png_view.exe test.png` and just look on it!

## Testing

Surprise again - just hit 'test.bat' and let it test!

It will load reference test PNG file and compare it with uncompressed reference BMP file.