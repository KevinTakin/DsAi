#ifndef GDI_H
#define GDI_H
#include <opencv2/opencv.hpp>
#include <windows.h>

class GDICapture
{
public:
    GDICapture() {};

    void init(int width, int height);

    cv::Mat capture();

private:
    int left;
    int top;
    int width;
    int height;
    BYTE* p;
    HDC sourceDC;
    HDC memoryDC;
    HBITMAP memoryBitmap;
    BITMAPINFOHEADER bitmapInfoHeader;

    BITMAP bitmap;

    void ReleaseResources();
};



#endif // !GDI_H
