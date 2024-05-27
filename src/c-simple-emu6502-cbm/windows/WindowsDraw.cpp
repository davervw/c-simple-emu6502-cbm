// WindowsDraw.cpp - UI handling for Windows
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

#ifdef _WINDOWS

#include "WindowsDraw.h"

ID2D1Factory* WindowsDraw::factory2d = 0;
ID2D1HwndRenderTarget* WindowsDraw::render2d = 0;
RECT WindowsDraw::clientRect{};
ID2D1Bitmap* WindowsDraw::bitmap = 0;
float WindowsDraw::pixelWidth = 0;
float WindowsDraw::pixelHeight = 0;
int WindowsDraw::screenWidth = 0;
int WindowsDraw::screenHeight = 0;
int WindowsDraw::borderWidth = 0;
int WindowsDraw::borderHeight = 0;
bool* WindowsDraw::redrawRequiredSignal;
HWND WindowsDraw::hWnd = 0;

bool WindowsDraw::ReRenderTarget()
{
    return CreateRenderTarget(screenWidth, screenHeight, borderWidth, borderHeight, *redrawRequiredSignal);
}

bool WindowsDraw::CreateRenderTarget(int screenwidth, int screenheight, int borderwidth, int borderheight, bool& redrawRequiredSignal)
{
    WindowsDraw::screenWidth = screenwidth;
    WindowsDraw::screenHeight = screenheight;
    WindowsDraw::borderWidth = borderwidth;
    WindowsDraw::borderHeight = borderheight;
    WindowsDraw::redrawRequiredSignal = &redrawRequiredSignal;
    redrawRequiredSignal = false;

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
    result = render2d->CreateBitmap(D2D1::SizeU(8, 8), bitmapProps, &bitmap); // TODO: allocate bitmap for entire screen containing objects (maybe not borders)
    if (result != S_OK) {
        OutputDebugStringA("CreateBitmap failed\n");
        return false;
    }

	pixelWidth = (clientRect.right - clientRect.left) / ((float)screenWidth + borderWidth * 2);
	pixelHeight = (clientRect.bottom - clientRect.top) / ((float)screenHeight + borderHeight * 2);

    //render2d->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // TODO: disable antialias if possible

    return true;
}

void WindowsDraw::DrawCharacter2Color(const byte* image, int x, int y, byte fg_red, byte fg_green, byte fg_blue, byte bg_red, byte bg_green, byte bg_blue)
{
    static byte raw[8 * 8 * 4]{};

    const unsigned char* p = image;
    for (int pixely = 0; pixely < 8; ++pixely) {
        byte value = 0;
        int pixelx = 0;
        for (byte bit = 128; bit != 0; bit >>= 1) {
            int i = pixely * 8 * 4 + pixelx * 4;
            if (*p & bit) {
                value |= bit;
                raw[i + 0] = fg_red;
                raw[i + 1] = fg_green;
                raw[i + 2] = fg_blue;
                raw[i + 3] = 255;
            }
            else {
                raw[i + 0] = bg_red;
                raw[i + 1] = bg_green;
                raw[i + 2] = bg_blue;
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

    float left = clientRect.left + (borderWidth + x * 8) * pixelWidth - 0.5f;
    float right = left + 8 * pixelWidth - 0.5f;
    float top = clientRect.top + (borderHeight + y * 8) * pixelHeight - 0.5f;
    float bottom = top + 8 * pixelHeight - 0.5f;
    render2d->DrawBitmap(
        bitmap,
        D2D1::RectF(left, top, right, bottom),
        1.0,
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        D2D1::RectF(0.0f, 0.0f, 8.0f, 8.0f)
    );
}

// TODO: Synchronize BeginDraw, EndDraw with CreateRenderTarget Release, Create... to avoid conflicts, e.g. updating screen and resizing at same time

void WindowsDraw::BeginDraw()
{
    //OutputDebugStringA("WindowsDraw::BeginDraw\n");
    render2d->BeginDraw();
}

void WindowsDraw::EndDraw()
{
    //OutputDebugStringA("WindowsDraw::EndDraw\n");
    render2d->EndDraw();
}

void WindowsDraw::DrawBorder(byte red, byte green, byte blue)
{
    ID2D1SolidColorBrush* brushBorder = 0;
    HRESULT result = render2d->CreateSolidColorBrush(D2D1::ColorF(red/255.0f, green/255.0f, blue/255.0f, 1), D2D1::BrushProperties(), &brushBorder); // TODO: emulator should initiate drawing its own borders with own colors
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
        float y1 = clientRect.top + pixelHeight * borderHeight;
        float x2 = x0;
        float y2 = y1 - 1; // minus one to avoid stitching so color is continuous
        float x3 = clientRect.right - pixelWidth * borderWidth;
        float y3 = y2;
        float x4 = clientRect.left + pixelWidth * borderWidth;
        float y4 = clientRect.bottom - pixelHeight * borderHeight;
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

        brushBorder->Release();
    }
}

void WindowsDraw::ClearScreen(byte red, byte green, byte blue)
{
    render2d->BeginDraw();
    render2d->Clear(D2D1::ColorF(red/255.0f, green/255.0f, blue/255.0f, 1));
    render2d->EndDraw();
}

void WindowsDraw::RenderPaint()
{
    if (redrawRequiredSignal != 0)
        *redrawRequiredSignal = true;
}

bool WindowsDraw::Init(HWND hWnd)
{
    WindowsDraw::hWnd = hWnd;
    HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &factory2d);
    if (result != S_OK) {
        FailBox("D2D1CreateFactory failed\n");
        return false;
    }
    return true;
}

void WindowsDraw::FailBox(const char* message)
{
    OutputDebugStringA(message);
    OutputDebugStringA("\n");
    MessageBoxA(NULL, message, "Cannot continue", MB_ICONEXCLAMATION | MB_OK | MB_SERVICE_NOTIFICATION);
}

#endif _WINDOWS
