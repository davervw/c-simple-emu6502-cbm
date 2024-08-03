// WindowsMain.cpp - WIN32 Application Entry point, Message Loop, and UI
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Unified Emulator for M5Stack/Teensy/ESP32 LCDs and Windows
//
// MIT License
//
// Copyright (c) 2024 by David R. Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

// Inspired by (not copied from) Creel Direct2D Tutorial 3: Direct 2D https://youtu.be/ng9Ho9qcVII?si=2a1ZSAaeYuGyosT5
// While I did follow the tutorial in another project, I revisited this effort from scratch on my own, and did not rely on the tutorial to recreate it in a different form.
// The results are notably similar due to shared experiences as well as similar requirements for a first example (clear, draw objects), yet the specifics are different.
// 
// Started with VS2022 C++ Windows Desktop template project that provided the huge bulk of the windows code
// Revised to include Direct2D factory and HWND render target inline, without implementing an external class
// Used Microsoft Learn, Stack Overflow, etc. to fill in the pieces of how to create and use objects
// Inferred from own experience that WM_SIZE handler was necessary

#ifdef _WINDOWS

#include "framework.h"
#include "c-simple-emu6502-cbm.h"
#include "WindowsDraw.h"
#include "WindowsKeyboard.h"
#include "WindowsStart.h"
#include <string.h>
#include <stdio.h>

const int MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

static DWORD childThread = 0;
static HANDLE hChildThread = 0;
static bool shuttingDown = false;
static bool isSizing = false;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CSIMPLEEMU6502CBM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

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
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_CSIMPLEEMU6502CBM));

    return RegisterClassExW(&wcex);
}


DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    HWND hWnd = (HWND)lpParameter;
    auto result = SendMessage(hWnd, WM_USER, 0, 0); // demo

    WindowsStart(hWnd, shuttingDown);

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
        WindowsDraw::FailBox("CreateWindowW failed");
        return FALSE;
    }

    if (!WindowsDraw::Init(hWnd))
        return FALSE;

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
//  WM_CREATE   - process main window creation
//  WM_DESTROY  - post a quit message and return
//  WM_KEYDOWN  - handle key press down
//  WM_KEYUP    - handle key press release
//  WM_PAINT    - Paint the main window
//  WM_SIZE     - handle notification that window has resized
//  WM_SYSKEYDOWN - handle system (with Alt) key down message
//  WM_SYSKEYUP - handle system (with Alt) key up message
//  WM_USER     - handle custom user message
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
        if (isSizing)
            return DefWindowProc(hWnd, message, wParam, lParam);
        int width = WindowsDraw::clientRect.right - WindowsDraw::clientRect.left;
        int height = WindowsDraw::clientRect.bottom - WindowsDraw::clientRect.top;
        if (width <= 0 || height <= 0)
            return DefWindowProc(hWnd, message, wParam, lParam);
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        WindowsDraw::RenderPaint(); // TODO: ask emulator to repaint its own screen
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_ENTERSIZEMOVE:
        isSizing = true;
        *WindowsDraw::redrawRequiredSignal = false; // try to avoid painting
        WindowsDraw::EndDraw();
        break;
    case WM_EXITSIZEMOVE:
        WindowsDraw::ReRenderTarget();
        WindowsDraw::BeginDraw();
        *WindowsDraw::redrawRequiredSignal = true;
        isSizing = false;
        break;
    //case WM_SIZE: // new dimensions mean we need a new renderer
    //    return DefWindowProc(hWnd, message, wParam, lParam);
    //    break;
    case WM_SYSKEYDOWN:
        if (wParam == VK_F4)
            return DefWindowProc(hWnd, message, wParam, lParam);
        if (wParam == 'Q')
            DestroyWindow(hWnd);
        // otherwise fall through
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        return WindowsKeyboard::ReceiveMessage(message, wParam, lParam);
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

#endif // _WINDOWS
