
#include "cap.h"
#include <iostream>
void CAP::init(int capture_width, int capture_height, int modes)
{
	mode = modes;
	if (mode == 0)
	{
		dxgi.init(capture_width, capture_height);
	}
	if (mode == 1)
	{
		gdi.init(capture_width, capture_height);
	}
}

//// 日志函数（调试用）
//static void LogMessage_1(const char* message) {
//	std::ofstream logfile("C:\\AIAgentLog1.txt", std::ios::app);
//	if (logfile.is_open()) {
//		logfile << message << std::endl;
//	}
//}

cv::Mat CAP::capture()
{
	if (mode == 0)
	{
		return dxgi.capture();
	}
	if (mode == 1)
	{
		return gdi.capture();
	}
	//LogMessage_1("1");
	return cv::Mat();
}