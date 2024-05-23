// My2D.cpp : Defines the entry point for the application.
//
// Inspired by (not copied from) Creel Direct2D Tutorial 3: Direct 2D https://youtu.be/ng9Ho9qcVII?si=2a1ZSAaeYuGyosT5
// While I did follow the tutorial in another project, I revisited this effort from scratch on my own, and did not rely on the tutorial to recreate it in a different form.
// The results are notably similar due to shared experiences as well as similar requirements for a first example (clear, draw objects), yet the specifics are different.
// 
// Started with VS2022 C++ Windows Desktop template project that provided the huge bulk of the windows code
// Revised to include Direct2D factory and HWND render target inline, without implementing an external class
// Used Microsoft Learn, Stack Overflow, etc. to fill in the pieces of how to create and use objects
// Inferred from own experience that WM_SIZE handler was necessary
//
// Copyright (c) 2022 by David R. Van Wagner
// github.com/davervw
// davevw.com
//
// MIT LICENSE

#include "framework.h"
#include "c-simple-emu6502-cbm.h"
#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")
#include <string.h>
#include <stdio.h>

const int MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

ID2D1Factory* factory2d = 0;
ID2D1HwndRenderTarget* render2d = 0;
RECT clientRect;
ID2D1Bitmap* bitmap = 0;
float pixelWidth = 0;
float pixelHeight = 0;
char chargen_rom[1024];
DWORD childThread = 0;
bool shuttingDown = false;
HANDLE hChildThread = 0;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
static void RenderC64Text(HWND hWnd, const char* s, int x, int y);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CSIMPLEEMU6502CBM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CSIMPLEEMU6502CBM));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CSIMPLEEMU6502CBM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CSIMPLEEMU6502CBM);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

static bool CreateRenderTarget(HWND hWnd)
{
    if (render2d != 0) {
        render2d->Release();
        render2d = 0;
    }

    if (!GetClientRect(hWnd, &clientRect)) {
        OutputDebugStringA("GetClientRect failed\n");
        return false;
    }

    // this size property cannot be ommited, otherwise render target doesn't seem to work
    D2D1_SIZE_U size = D2D1::SizeU(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    //D2D1_SIZE_U size = D2D1::SizeU(320, 200);

    D2D1_PIXEL_FORMAT pixelFormat{};
    pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;

    HRESULT result = factory2d->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, pixelFormat),
        D2D1::HwndRenderTargetProperties(hWnd, size),
        &render2d);
    if (result != S_OK) {
        OutputDebugStringA("CreateHwndRenderTarget failed\n");
        return false;
    }

    // Thanks to https://www.codeproject.com/Questions/5368277/Directdraw-surface-w-bitmap-not-rendering
    D2D1_BITMAP_PROPERTIES bitmapProps{};
    render2d->GetDpi(&bitmapProps.dpiX, &bitmapProps.dpiY);
    bitmapProps.dpiX = 0;
    bitmapProps.dpiY = 0;
    bitmapProps.pixelFormat = pixelFormat;
    result = render2d->CreateBitmap(D2D1::SizeU(8, 8), bitmapProps, &bitmap);
    if (result != S_OK) {
        OutputDebugStringA("CreateBitmap failed\n");
        return false;
    }

    pixelWidth = (clientRect.right - clientRect.left) / (320.0f + 64);
    pixelHeight = (clientRect.bottom - clientRect.top) / (200.0f + 64);

    //render2d->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    return true;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    HWND hWnd = (HWND)lpParameter;
    auto result = SendMessage(hWnd, WM_USER, 0, 0);

    extern void start(bool&);
    start(shuttingDown);

    OutputDebugStringA("Exiting thread\n");

    return 0;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        OutputDebugStringA("CreateWindowW failed\n");
        return FALSE;
    }

    HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &factory2d);
    if (result != S_OK) {
        OutputDebugStringA("D2D1CreateFactory failed\n");
        return false;
    }

    if (!CreateRenderTarget(hWnd)) {
        OutputDebugStringA("CreateRenderTarget failed\n");
        return FALSE;
    }

    FILE* f = 0;
    fopen_s(&f, "roms/minimum/asciifont.bin", "rb");
    if (f == 0)
        return FALSE;
    auto bytes = fread(chargen_rom, 1, sizeof(chargen_rom), f);
    fclose(f);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//  WM_SIZE     - handle notification that window has resized
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        hChildThread = CreateThread(NULL, 0, ThreadProc, (LPVOID*)hWnd, 0, &childThread);
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        if (width <= 0 || height <= 0)
            return DefWindowProc(hWnd, message, wParam, lParam);
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        render2d->BeginDraw();
        render2d->Clear(D2D1::ColorF(0, 0, 1, 1));
        ID2D1SolidColorBrush* brushBorder = 0;
        HRESULT result = render2d->CreateSolidColorBrush(D2D1::ColorF(0.4f, 0.5f, 1.0f, 1), D2D1::BrushProperties(), &brushBorder);
        if (brushBorder != 0)
        {
            // x0,y0---------------------------+
            // x2,y2-------------------x3,y3 x1,y1
            //   |     |                 |     |
            // x6,y6 x4,y4-------------------x5,y5
            //   +---------------------------x7,y7

            float x0 = (float)clientRect.left;
            float y0 = (float)clientRect.top;
            float x1 = (float)clientRect.right;
            float y1 = clientRect.top + pixelHeight * 32;
            float x2 = x0;
            float y2 = y1-1; // minus one to avoid stitching so color is continuous
            float x3 = clientRect.right - pixelWidth * 32;
            float y3 = y2;
            float x4 = clientRect.left + pixelWidth * 32;
            float y4 = clientRect.bottom - pixelHeight * 32;
            float x5 = x1;
            float y5 = y4;
            float x6 = x0;
            float y6 = y4;
            float x7 = (float)clientRect.right;
            float y7 = (float)clientRect.bottom;

            render2d->FillRectangle(D2D1::RectF(x0, y0, x1, y1), brushBorder);
            render2d->FillRectangle(D2D1::RectF(x2, y2, x4, y7), brushBorder);
            render2d->FillRectangle(D2D1::RectF(x3, y3, x5, y7), brushBorder);
            render2d->FillRectangle(D2D1::RectF(x6, y6, x7, y7), brushBorder);
        }
        RenderC64Text(hWnd, "**** COMMODORE 64 BASIC V2 ****", 4, 1);
        RenderC64Text(hWnd, "64K RAM SYSTEM  38911 BASIC BYTES FREE", 1, 3);
        RenderC64Text(hWnd, "READY.", 0, 5);
        RenderC64Text(hWnd, "\xA0", 0, 6);

        if (brushBorder != 0)
            brushBorder->Release();
        render2d->EndDraw();
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_SIZE: // new dimensions mean we need a new renderer
        if (!CreateRenderTarget(hWnd))
            return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_USER:
    {
        //auto id = GetCurrentThreadId();
        //wchar_t buffer[80];
        //if (_snwprintf_s(buffer, sizeof(buffer), _T("WM_USER thread id %d\n"), (int)id) <= 0)
        //    buffer[0] = 0;
        //OutputDebugStringW(buffer);
        //MessageBox(hWnd, _T("WM_USER"), _T("WndProc"), MB_OK);
        return 1234;
    }
    case WM_DESTROY:
    {
        shuttingDown = true;
        WaitForSingleObject(hChildThread, 1000);
        OutputDebugStringA("Exiting main Window\n");
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

static void RenderC64Char(HWND hWnd, char c, int x, int y, bool inverse = FALSE)
{
    static byte raw[8 * 8 * 4]{};

    if (c < 0) {
        c = (unsigned char)c - 128;
        inverse = !inverse;
    }

    const char* p = &chargen_rom[(unsigned char)c * 8];
    for (int pixely = 0; pixely < 8; ++pixely) {
        byte value = 0;
        int pixelx = 0;
        for (byte bit = 128; bit != 0; bit >>= 1) {
            int i = pixely * 8 * 4 + pixelx * 4;
            if ((*p & bit) ? !inverse : inverse) {
                value |= bit;
                raw[i + 0] = 255;
                raw[i + 1] = 255;
                raw[i + 2] = 255;
                raw[i + 3] = 255;
            }
            else {
                raw[i + 0] = 0;
                raw[i + 1] = 0;
                raw[i + 2] = 255;
                raw[i + 3] = 255;
            }
            ++pixelx;
        }
        ++p;
    }

    auto bitmapRect = D2D1::RectU(0, 0, 8, 8);
    HRESULT result = bitmap->CopyFromMemory(&bitmapRect, raw, 8 * 4);
    if (result != S_OK)
        return;

    float left = clientRect.left + (32 + x * 8) * pixelWidth - 0.5f;
    float right = left + 8 * pixelWidth - 0.5f;
    float top = clientRect.top + (32 + y * 8) * pixelHeight - 0.5f;
    float bottom = top + 8 * pixelHeight - 0.5f;
    render2d->DrawBitmap(
        bitmap,
        D2D1::RectF(left, top, right, bottom),
        1.0,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        D2D1::RectF(0.0f, 0.0f, 8.0f, 8.0f)
    );
}

static void RenderC64Text(HWND hWnd, const char* s, int x, int y)
{
    while (s != 0 && *s != 0)
        RenderC64Char(hWnd, *s++, x++, y);
}
