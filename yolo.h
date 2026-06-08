#ifndef YOLO_H
#define YOLO_H

#include "yolov5.h"
#include "yolox.h"

class YOLO
{
public:

	void init(const char* param_path, const char* bin_path, const int numThreads, int modes);
	void detect(const cv::Mat& img, std::vector<Object>& result, int imgsize, float conf, float iou);

private:
	int mode;
	YoloV5Detector yolov5;
	YoloXDetector yolox;
};



#endif