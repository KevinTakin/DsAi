#include "yolo.h"


void YOLO::init(const char* param_path, const char* bin_path, const int numThreads, int modes)
{
	
	mode = modes;
	if (mode == 0)
	{
		yolov5.init(param_path, bin_path, numThreads);
	}
	else if (mode == 1)
	{
		yolox.init(param_path, bin_path, numThreads);
	}
}

void YOLO::detect(const cv::Mat& img, std::vector<Object>& result, int imgsize, float conf, float iou)
{
	if (mode == 0)
	{
		yolov5.detect(img, result, imgsize, conf, iou);
	}
	else if (mode == 1)
	{
		yolox.detect(img, result, imgsize, conf, iou);
	}
}
