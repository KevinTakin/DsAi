#ifndef CAP_H
#define CAP_H

#include "cap_dx.h"
#include "cap_gdi.h"


class CAP
{
public:
	CAP() {};
	void init(int capture_width, int capture_height, int modes);
	cv::Mat capture();

private:
	int mode = -1;
	DXScreenCapture dxgi;
    GDICapture gdi;
};




#endif