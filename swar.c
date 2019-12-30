// Just a silly and slow exercise of some the more advanced parts of SCC
#include "lib.h"

enum {
    VIDEO_SEG = 0xb800,
    FONT_SEG  = 0xffa6,
    FONT_OFF  = 0x000e,
    SCREENW   = 320,
    SCREENH   = 200,
};

enum {
    INT10H_SETMODE = 0x0000,
    INT10H_SETPAL  = 0x0B00,
    INT10H_GETMODE = 0x0F00,
};

#ifdef __SCC__
int Int10h(int ax, int bx)
{
    _emit 0x8B _emit 0x46 _emit 0x04    // MOV AX, [BP+4]
    _emit 0x8B _emit 0x5E _emit 0x06    // MOV BX, [BP+6]
    _emit 0xCD _emit 0x10               // INT 0x10
}

int KeyPressed()
{
    _emit 0xB4 _emit 0x01               // MOV AH, 1
    _emit 0xCD _emit 0x16               // INT 0x16
    _emit 0xB8 _emit 0x00 _emit 0x00    // MOV AX, 0
    _emit 0x74 _emit 0x01               // JZ  $+3
    _emit 0x40                          // INC AX
}

unsigned ReadKey()
{
    _emit 0xB4 _emit 0x00               // MOV AH, 0
    _emit 0xCD _emit 0x16               // INT 0x16
}

int inb(int port)
{
    _emit 0x8B _emit 0x56 _emit 0x04    // MOV DX, [BP+4]
    _emit 0xEC                          // IN AL, DX
    _emit 0x98                          // CBW
}

void ScreenUpdate()
{
    while (!(inb(0x3DA) & 8))
        ;
}

#else

static unsigned char VideoMem[16384];
static char Initialized;

static const unsigned char Font[1024] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x81, 0xa5, 0x81, 0xbd, 0x99, 0x81, 0x7e,
0x7e, 0xff, 0xdb, 0xff, 0xc3, 0xe7, 0xff, 0x7e, 0x6c, 0xfe, 0xfe, 0xfe, 0x7c, 0x38, 0x10, 0x00,
0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x10, 0x08, 0x38, 0x7c, 0x38, 0xfe, 0xfe, 0x7c, 0x38, 0x7c,
0x10, 0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x7c, 0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00,
0xff, 0xff, 0xe7, 0xc3, 0xc3, 0xe7, 0xff, 0xff, 0x00, 0x3c, 0x66, 0x42, 0x42, 0x66, 0x3c, 0x00,
0xff, 0xc3, 0x99, 0xbd, 0xbd, 0x99, 0xc3, 0xff, 0x0f, 0x07, 0x0f, 0x7d, 0xcc, 0xcc, 0xcc, 0x78,
0x3c, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x7e, 0x18, 0x3f, 0x33, 0x3f, 0x30, 0x30, 0x70, 0xf0, 0xe0,
0x7f, 0x63, 0x7f, 0x63, 0x63, 0x67, 0xe6, 0xc0, 0x99, 0x5a, 0x3c, 0xe7, 0xe7, 0x3c, 0x5a, 0x99,
0x80, 0xe0, 0xf8, 0xfe, 0xf8, 0xe0, 0x80, 0x00, 0x02, 0x0e, 0x3e, 0xfe, 0x3e, 0x0e, 0x02, 0x00,
0x18, 0x3c, 0x7e, 0x18, 0x18, 0x7e, 0x3c, 0x18, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x00,
0x7f, 0xdb, 0xdb, 0x7b, 0x1b, 0x1b, 0x1b, 0x00, 0x3e, 0x63, 0x38, 0x6c, 0x6c, 0x38, 0xcc, 0x78,
0x00, 0x00, 0x00, 0x00, 0x7e, 0x7e, 0x7e, 0x00, 0x18, 0x3c, 0x7e, 0x18, 0x7e, 0x3c, 0x18, 0xff,
0x18, 0x3c, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x3c, 0x18, 0x00,
0x00, 0x18, 0x0c, 0xfe, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x30, 0x60, 0xfe, 0x60, 0x30, 0x00, 0x00,
0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xfe, 0x00, 0x00, 0x00, 0x24, 0x66, 0xff, 0x66, 0x24, 0x00, 0x00,
0x00, 0x18, 0x3c, 0x7e, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x7e, 0x3c, 0x18, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x78, 0x78, 0x30, 0x30, 0x00, 0x30, 0x00,
0x6c, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x6c, 0xfe, 0x6c, 0xfe, 0x6c, 0x6c, 0x00,
0x30, 0x7c, 0xc0, 0x78, 0x0c, 0xf8, 0x30, 0x00, 0x00, 0xc6, 0xcc, 0x18, 0x30, 0x66, 0xc6, 0x00,
0x38, 0x6c, 0x38, 0x76, 0xdc, 0xcc, 0x76, 0x00, 0x60, 0x60, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x18, 0x30, 0x60, 0x60, 0x60, 0x30, 0x18, 0x00, 0x60, 0x30, 0x18, 0x18, 0x18, 0x30, 0x60, 0x00,
0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x80, 0x00,
0x7c, 0xc6, 0xce, 0xde, 0xf6, 0xe6, 0x7c, 0x00, 0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x00,
0x78, 0xcc, 0x0c, 0x38, 0x60, 0xcc, 0xfc, 0x00, 0x78, 0xcc, 0x0c, 0x38, 0x0c, 0xcc, 0x78, 0x00,
0x1c, 0x3c, 0x6c, 0xcc, 0xfe, 0x0c, 0x1e, 0x00, 0xfc, 0xc0, 0xf8, 0x0c, 0x0c, 0xcc, 0x78, 0x00,
0x38, 0x60, 0xc0, 0xf8, 0xcc, 0xcc, 0x78, 0x00, 0xfc, 0xcc, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x00,
0x78, 0xcc, 0xcc, 0x78, 0xcc, 0xcc, 0x78, 0x00, 0x78, 0xcc, 0xcc, 0x7c, 0x0c, 0x18, 0x70, 0x00,
0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x60,
0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0xfc, 0x00, 0x00,
0x60, 0x30, 0x18, 0x0c, 0x18, 0x30, 0x60, 0x00, 0x78, 0xcc, 0x0c, 0x18, 0x30, 0x00, 0x30, 0x00,
0x7c, 0xc6, 0xde, 0xde, 0xde, 0xc0, 0x78, 0x00, 0x30, 0x78, 0xcc, 0xcc, 0xfc, 0xcc, 0xcc, 0x00,
0xfc, 0x66, 0x66, 0x7c, 0x66, 0x66, 0xfc, 0x00, 0x3c, 0x66, 0xc0, 0xc0, 0xc0, 0x66, 0x3c, 0x00,
0xf8, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0xf8, 0x00, 0xfe, 0x62, 0x68, 0x78, 0x68, 0x62, 0xfe, 0x00,
0xfe, 0x62, 0x68, 0x78, 0x68, 0x60, 0xf0, 0x00, 0x3c, 0x66, 0xc0, 0xc0, 0xce, 0x66, 0x3e, 0x00,
0xcc, 0xcc, 0xcc, 0xfc, 0xcc, 0xcc, 0xcc, 0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00,
0x1e, 0x0c, 0x0c, 0x0c, 0xcc, 0xcc, 0x78, 0x00, 0xe6, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0xe6, 0x00,
0xf0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xfe, 0x00, 0xc6, 0xee, 0xfe, 0xfe, 0xd6, 0xc6, 0xc6, 0x00,
0xc6, 0xe6, 0xf6, 0xde, 0xce, 0xc6, 0xc6, 0x00, 0x38, 0x6c, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00,
0xfc, 0x66, 0x66, 0x7c, 0x60, 0x60, 0xf0, 0x00, 0x78, 0xcc, 0xcc, 0xcc, 0xdc, 0x78, 0x1c, 0x00,
0xfc, 0x66, 0x66, 0x7c, 0x6c, 0x66, 0xe6, 0x00, 0x78, 0xcc, 0xe0, 0x70, 0x1c, 0xcc, 0x78, 0x00,
0xfc, 0xb4, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xfc, 0x00,
0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x78, 0x30, 0x00, 0xc6, 0xc6, 0xc6, 0xd6, 0xfe, 0xee, 0xc6, 0x00,
0xc6, 0xc6, 0x6c, 0x38, 0x38, 0x6c, 0xc6, 0x00, 0xcc, 0xcc, 0xcc, 0x78, 0x30, 0x30, 0x78, 0x00,
0xfe, 0xc6, 0x8c, 0x18, 0x32, 0x66, 0xfe, 0x00, 0x78, 0x60, 0x60, 0x60, 0x60, 0x60, 0x78, 0x00,
0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x02, 0x00, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00,
0x10, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0c, 0x7c, 0xcc, 0x76, 0x00,
0xe0, 0x60, 0x60, 0x7c, 0x66, 0x66, 0xdc, 0x00, 0x00, 0x00, 0x78, 0xcc, 0xc0, 0xcc, 0x78, 0x00,
0x1c, 0x0c, 0x0c, 0x7c, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x78, 0xcc, 0xfc, 0xc0, 0x78, 0x00,
0x38, 0x6c, 0x60, 0xf0, 0x60, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x76, 0xcc, 0xcc, 0x7c, 0x0c, 0xf8,
0xe0, 0x60, 0x6c, 0x76, 0x66, 0x66, 0xe6, 0x00, 0x30, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00,
0x0c, 0x00, 0x0c, 0x0c, 0x0c, 0xcc, 0xcc, 0x78, 0xe0, 0x60, 0x66, 0x6c, 0x78, 0x6c, 0xe6, 0x00,
0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00, 0x00, 0x00, 0xcc, 0xfe, 0xfe, 0xd6, 0xc6, 0x00,
0x00, 0x00, 0xf8, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x78, 0xcc, 0xcc, 0xcc, 0x78, 0x00,
0x00, 0x00, 0xdc, 0x66, 0x66, 0x7c, 0x60, 0xf0, 0x00, 0x00, 0x76, 0xcc, 0xcc, 0x7c, 0x0c, 0x1e,
0x00, 0x00, 0xdc, 0x76, 0x66, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x7c, 0xc0, 0x78, 0x0c, 0xf8, 0x00,
0x10, 0x30, 0x7c, 0x30, 0x30, 0x34, 0x18, 0x00, 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0xcc, 0x76, 0x00,
0x00, 0x00, 0xcc, 0xcc, 0xcc, 0x78, 0x30, 0x00, 0x00, 0x00, 0xc6, 0xd6, 0xfe, 0xfe, 0x6c, 0x00,
0x00, 0x00, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0xcc, 0xcc, 0xcc, 0x7c, 0x0c, 0xf8,
0x00, 0x00, 0xfc, 0x98, 0x30, 0x64, 0xfc, 0x00, 0x1c, 0x30, 0x30, 0xe0, 0x30, 0x30, 0x1c, 0x00,
0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00, 0xe0, 0x30, 0x30, 0x1c, 0x30, 0x30, 0xe0, 0x00,
0x76, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x6c, 0xc6, 0xc6, 0xfe, 0x00
};

#ifdef _WIN32
#include <windows.h>

static HWND MainWindow;
static HDC hdc;
static HBITMAP hbm;
static unsigned char Bitmap[SCREENW*SCREENH];
static DWORD LastFrame;
enum { KBBUF_SIZE = 16 };
static unsigned short KeyboardBuffer[KBBUF_SIZE];
static unsigned KBHead, KBTail;

static void ScreenUpdate(void)
{
    MSG msg;

    unsigned char* dst = Bitmap;
    for (int y = 0; y < SCREENH; ++y) {
        const unsigned char* src = &VideoMem[(y>>1)*80 + (y&1?8192:0)];
        for (int x = 0; x < SCREENW/4; ++x) {
            const unsigned char p = *src++;
            *dst++ = (p>>6)&3;
            *dst++ = (p>>4)&3;
            *dst++ = (p>>2)&3;
            *dst++ = (p>>0)&3;
        }
    }

    const DWORD CurTime = GetTickCount();
    if (LastFrame && CurTime - LastFrame < 20) {
        Sleep(LastFrame+20-CurTime);
    }

    InvalidateRect(MainWindow, NULL, TRUE);
    LastFrame = GetTickCount();
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            exit(0);
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static int KeyPressed(void)
{
    ScreenUpdate();
    return KBHead != KBTail;
}

static unsigned ReadKey(void)
{
    if (KBHead == KBTail) {
        while (!KeyPressed())
            ;
    }
    return KeyboardBuffer[KBHead++ % KBBUF_SIZE];
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg) {
    case WM_CREATE:
        {
            HDC whdc = GetWindowDC(hwnd);
            hdc = CreateCompatibleDC(whdc);
            hbm = CreateCompatibleBitmap(whdc, SCREENW, SCREENH);
            SelectObject(hdc, hbm);
            ReleaseDC(hwnd, whdc);
        }
        break;
    case WM_DESTROY:
        DeleteObject(hbm);
        DeleteDC(hdc);
        PostQuitMessage(0);
        break;
    case WM_KEYUP:
        {
            // Hack to roughly match BIOS output
            if (!isdigit(wparam) && (wparam < 'A' || wparam > 'Z') && wparam != 0x0D && wparam != 0x1B && wparam != ' ')
                wparam = 0;
            if (KBTail - KBHead < sizeof(KeyboardBuffer)) {
                KeyboardBuffer[KBTail++ % KBBUF_SIZE] = (wparam&0xff)|((lparam>>8)&0xff00);
            }
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            if (BeginPaint(hwnd, &ps) && !IsRectEmpty(&ps.rcPaint)) {
                union {
                    BITMAPINFO bmi;
                    char       buffer[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256];
                } u;
                ZeroMemory(&u, sizeof(u));
                u.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                u.bmi.bmiHeader.biWidth  = SCREENW;
                u.bmi.bmiHeader.biHeight = -SCREENH;
                u.bmi.bmiHeader.biPlanes = 1;
                u.bmi.bmiHeader.biCompression = BI_RGB;
                u.bmi.bmiHeader.biBitCount = 8;
                const RGBQUAD pal[4] = {
                    {0x00, 0x00, 0x00, 0xff},
                    {0x55, 0xff, 0x55, 0xff},
                    {0x55, 0x55, 0xff, 0xff},
                    {0x55, 0xff, 0xff, 0xff},
                };
                memcpy(&u.bmi.bmiColors, pal, sizeof(pal));
                if (!SetDIBits(hdc, hbm, 0, SCREENH, Bitmap, &u.bmi, DIB_RGB_COLORS))
                    assert(0);

                RECT r;
                GetClientRect(hwnd, &r);
                StretchBlt(ps.hdc, 0, 0, r.right - r.left, r.bottom - r.top, hdc, 0, 0, SCREENW, SCREENH, SRCCOPY);
                EndPaint(hwnd, &ps);
            }
        }
        return 0;
    }
    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

static int Init(void)
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &WindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = TEXT("Main window");
    if (!RegisterClass(&wc)) {
        printf("Error registering class: %d\n", (int)GetLastError());
        return 0;
    }
    RECT r = { 0, 0, SCREENW*2, SCREENH*2 };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    MainWindow = CreateWindow(wc.lpszClassName, TEXT("SWAR"), WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, 0, 0, wc.hInstance, 0);
    if (!MainWindow) {
        printf("Error creating window: %d\n", (int)GetLastError());
        return 0;
    }
    return 1;
}

static void Cleanup(void)
{
    if (IsWindow(MainWindow))
        DestroyWindow(MainWindow);
}
#else
#error Not implemented
#endif

static int Int10h(int ax, int bx) {
    if (!Initialized) {
        if (!Init())
            exit(1);
        atexit(&Cleanup);
        Initialized = 1;
    }
    (void)ax; (void)bx; return 0;
}

#define LINADDR(seg, off) ((unsigned)(((seg)&0xffff)*16+((off)&0xffff)) & 0xfffff)

unsigned char* GetPtr(int seg, int off)
{
    const unsigned addr = LINADDR(seg, off);
    if (addr >= LINADDR(VIDEO_SEG, 0) && addr < LINADDR(VIDEO_SEG, sizeof(VideoMem))) {
        return &VideoMem[addr-16*VIDEO_SEG];
    }
    if (addr >= LINADDR(FONT_SEG, FONT_OFF) && addr < LINADDR(FONT_SEG, FONT_OFF+sizeof(Font))) {
        return (unsigned char*)&Font[addr - LINADDR(FONT_SEG, FONT_OFF)];
    }
    abort();
}

static int FarPeek(int seg, int off)
{
    return *GetPtr(seg, off);
}

static void FarPoke(int seg, int off, int val)
{
    *GetPtr(seg, off) = (unsigned char)val;
}
#endif

const unsigned char CosTab[] = "\x00\xFF\xFF\xFF\xFE\xFE\xFD\xFC\xFB\xF9\xF8\xF6\xF4\xF3\xF1\xEE\xEC\xEA\xE7\xE4\xE1\xDE\xDB\xD8\xD4\xD1\xCD\xC9\xC5\xC1\xBD\xB9\xB5\xB0\xAB\xA7\xA2\x9D\x98\x93\x8E\x88\x83\x7E\x78\x73\x6D\x67\x61\x5C\x56\x50\x4A\x44\x3E\x38\x31\x2B\x25\x1F\x19\x12\x0C\x06";
int ICos(int ang)
{
    const int q = ang>>6&3;
    int r = CosTab[(q&1?-ang:ang)&63];
    if (!r && !(q&1))
        r |= 0x100;
    return q>>1^(q&1) ? -r : r;
}

int ISin(int ang)
{
    return ICos(ang - 64);
}

struct Tank {
    int x, y, ang, pow;
    char* c;
};


int YTab[SCREENH];
char XMask[4];
char Col[4][4];

int Grass[SCREENW];

int Seed = 43;
int Rand(void)
{
    int b = Seed & 1;
    Seed >>= 1;
    if (b) Seed ^= 0xb40;
    return Seed & 0x7fff;
}

int ReadPixel(int x, int y)
{
    int r = (3-(x&3))<<1;
    x >>= 2;
    x += YTab[y];
    return (FarPeek(VIDEO_SEG, x) >> r) & 3;
}

void PutPixel(int x, int y, char* cs)
{
    int r = x&3;
    x >>= 2;
    x += YTab[y];
    int old = FarPeek(VIDEO_SEG, x);
    FarPoke(VIDEO_SEG, x, (old&XMask[r])|cs[r]);
}

enum { TW=9, TH=4, TUR=6 };
void DrawTurret(int x, int y, int ang, char* c)
{
    int tx = (x+TW/2)<<7;
    int ty = (y+TUR)<<7;
    int ca = ICos(ang)>>1;
    int sa = -ISin(ang)>>1;
    int i;
    for (i = 0; i < TUR; ++i) {
        PutPixel((tx>>7)&0x1ff, ty>>7, c);
        tx += ca;
        ty += sa;
    }
}

#define INSCREEN(sx,sy) ((sx)>=0&&(sx)<SCREENW&&(sy)>=0&&(sy)<SCREENH)

int Fire(struct Tank* t)
{
    int ca = ICos(t->ang)>>1;
    int sa = -ISin(t->ang)>>1;

    int tx = ((t->x+TW/2)<<7) + ca*TUR;
    int ty = ((t->y+TUR)<<7)  + sa*TUR;

    int lx=-1, ly=0;

    ca = ca * t->pow / 100;
    sa = sa * t->pow / 100;

    int ssa = 0;

    for (;;) {
        int sx = (tx>>7)&0x1ff;
        int sy = ty>>7;

        if (sx < 0 || sx >= SCREENW || sy >= SCREENH)
            break;

        ScreenUpdate();

        if (INSCREEN(lx, ly))
            PutPixel(lx, ly, Col[0]);

        if (INSCREEN(sx, sy)) {
            if (sy > 8) {
                const int hit = ReadPixel(sx, sy);
                if (hit) {
                    PutPixel(sx, sy, Col[0]);
                    return hit;
                }
            }
            PutPixel(sx, sy, Col[1]);
        }

        lx = sx;
        ly = sy;

        tx += ca;
        ty += sa;

        sa += ssa>>10;

        ssa += 5;
    }

    if (INSCREEN(lx, ly))
        PutPixel(lx, ly, Col[0]);

    return 0;
}

void DrawBody(int x, int y, char* c)
{
    int tx, ty;
    for (ty = 0; ty < TH; ++ty) {
        for (tx = 0; tx < TW; ++tx) {
            PutPixel(x+tx, y+ty+(TUR+1), c);
        }
    }
}

void DrawChar(int x, int y, int ch, char* fg, char* bg)
{
    int cx, cy;
    for (cy = 0; cy < 8; ++cy) {
        int row = FarPeek(FONT_SEG, FONT_OFF+ch*8+cy);
        for (cx = 0; cx < 8; ++cx) {
            PutPixel(x+cx, y+cy, row&0x80 ? fg : bg);
            row <<= 1;
        }
    }
}

void DrawStr(int x, int y, const char* str, char* fg, char* bg)
{
    while (*str) {
        DrawChar(x, y, *str++, fg, bg);
        x += 8;
    }
}
void ShowTankInfo(struct Tank* t)
{
    char temp[32];
    sprintf(temp, "Ang %3d  Pwr %3d", t->ang*180>>7, t->pow);
    DrawStr(0, 0, temp, t->c, Col[0]);
}

void DrawTank(struct Tank* t)
{
    DrawBody(t->x, t->y, t->c);
    DrawTurret(t->x, t->y, t->ang, t->c);
}

void UpdateTurrent(struct Tank* t, int ang)
{
    // Erase old turrent
    DrawTurret(t->x, t->y, t->ang, Col[0]);
    if (ang < 0)
        ang += 129;
    if (ang > 128)
        ang -= 129;
    DrawTurret(t->x, t->y, t->ang = ang, t->c);
}

int main()
{
#ifdef __SCC__
    Seed = clock();
#endif

    const int OldMode = Int10h(INT10H_GETMODE, 0) & 0xff;
    Int10h(INT10H_SETMODE|4, 0x0000); // Set 320x200 2-bit color mode
    Int10h(INT10H_SETPAL,    0x0100); // Set alternate palette

    int x, y;

    for (y = 0; y < SCREENH; ++y) {
        YTab[y] = (y>>1)*80 | (y&1)<<13;
    }

    for (x = 0; x < 4; ++x) {
        int shift = 6-(x&3)*2;
        XMask[x] = ~(3<<shift);
        for (y = 0; y < 4; ++y) {
            Col[y][x] = y << shift;
        }
    }

    y = SCREENH/2;
    for (x = 0; x < SCREENW; ++x) {
        if (y < 20) y = 20;
        else if (y > SCREENH-20) y = SCREENH-20;
        Grass[x] = y;
        int s;
        do {
            s = Rand() & 7;
        } while (s == 7);
        y += s-3;
    }

    for (x=0; x<SCREENW; ++x) {
        for (y = Grass[x]; y < 200; ++y) {
            PutPixel(x, y, Col[1]);
        }
    }

    char tmp[32];

    struct Tank t1, t2;
    t1.x = 20;
    t1.y = Grass[t1.x+TW/2]-(TH+TUR+1);
    t1.ang = 32;
    t1.pow = 100;
    t1.c = Col[2];

    t2.x = 300;
    t2.y = Grass[t2.x+TW/2]-(TH+TUR+1);
    t2.ang = 96;
    t2.pow = 100;
    t2.c = Col[3];

    DrawTank(&t1);
    DrawTank(&t2);
    struct Tank* cur = &t1;
    ShowTankInfo(cur);
    for (;;) {
        if (KeyPressed()) {
            const int ch = ReadKey();
            //sprintf(tmp, "Pressed: %04X", ch);
            //DrawStr(320-(1+(int)strlen(tmp))*8, 0, tmp, Col[1], Col[0]);

            switch (ch&0xff) {
            case 27: goto Done;
            case ' ':
            case 13:
                {
                    const int hit = Fire(cur);
                    if (hit == 2 || hit == 3) {
                        sprintf(tmp, "Player %d wins!", 4-hit);
                        DrawStr((SCREENW-8*(int)strlen(tmp))/2, SCREENH/2-4, tmp, Col[hit^1], Col[0]);
                        ReadKey();
                        goto Done;
                    }
                    cur = cur == &t1 ? &t2: &t1;
                    ShowTankInfo(cur);
                    continue;
                }
            }

            int ang = cur->ang;
            switch (ch) {
            case 0x4B00:
                ++ang;
                break;
            case 0x4D00:
                --ang;
                break;
            case 0x4800:
                if (++cur->pow >= 100)
                    cur->pow = 100;
                break;
            case 0x5000:
                if (--cur->pow < 0)
                    cur->pow = 0;
                break;
            case 0x4900:
                cur->pow = 100;
                break;
            case 0x5100:
                cur->pow = 0;
                break;
            default:
                continue;
            }

            if (ang != cur->ang)
                UpdateTurrent(cur, ang);
            ShowTankInfo(cur);
        }
    }
Done:
    Int10h(INT10H_SETMODE|OldMode, 0);
}
