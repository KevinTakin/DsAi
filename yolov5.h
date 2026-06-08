#ifndef YOLOV5_H
#define YOLOV5_H
#include "ncnn\layer.h"
#include "ncnn\net.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include "mokuai.h"

class YoloV5Detector {
public:
    YoloV5Detector() {};

    void init(const char* param_path, const char* bin_path, const int numThreads);

    int detect(const cv::Mat& img, std::vector<Object>& result, int imgsize, float conf, float iou);

private:
    ncnn::Net yolov5;


    static inline int num_threads;

    static inline float intersection_area(const Object& a, const Object& b) noexcept;

    static void qsort_descent_inplace(std::vector<Object>& objects, int left, int right);

    static void qsort_descent_inplace(std::vector<Object>& objects);

    static void nms_sorted_bboxes(const std::vector<Object>& objects, std::vector<int>& picked, float nms_threshold, bool agnostic = true);

    static inline float sigmoid_yolov5(float x);

    static void generate_proposals_yolov5(const ncnn::Mat& anchors, int stride, const ncnn::Mat& in_pad,
        const ncnn::Mat& feat_blob, float prob_threshold, std::vector<Object>& proposals);
};




#endif // YOLOV5_H