
#include "cap_gdi.h"

void GDICapture::init(int width, int height)
{
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;

    left = centerX - width / 2;
    top = centerY - height / 2;

    this->width = width;
    this->height = height;
    p = new BYTE[width * height * 3];
    sourceDC = GetDC(GetDesktopWindow());
    memoryDC = CreateCompatibleDC(sourceDC);

    bitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfoHeader.biBitCount = 24;
    bitmapInfoHeader.biCompression = 0;
    bitmapInfoHeader.biHeight = -height;
    bitmapInfoHeader.biPlanes = 1;
    bitmapInfoHeader.biWidth = width;

    memoryBitmap = CreateDIBSection(sourceDC, (LPBITMAPINFO)&bitmapInfoHeader, DIB_RGB_COLORS, (VOID**)&p, NULL, 0);

    SelectObject(memoryDC, memoryBitmap);

};

cv::Mat GDICapture::capture()
{

    BitBlt(memoryDC, 0, 0, width, height, sourceDC, left, top, SRCCOPY);
    GetObject(memoryBitmap, sizeof bitmap, &bitmap);

    cv::Mat image(height, width, CV_8UC3, bitmap.bmBits);
    cv::cvtColor(image, image, cv::COLOR_BGRA2BGR);

    return image;
};

void GDICapture::ReleaseResources()
{
    delete[] p;
    DeleteObject(memoryBitmap);
    DeleteDC(memoryDC);
    ReleaseDC(FindWindowA(NULL, NULL), sourceDC);
};