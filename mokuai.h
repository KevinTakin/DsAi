#pragma once

#include <opencv2/opencv.hpp>

struct Object
{
    cv::Rect_<float> rect;
    int label;
    double prob;
    int track_id;


};

struct Aim
{
    int label;
    double origin_x;
    double origin_y;
    double height;
    double width;
    double centerX;
    double centerY;
    int track_id;                      // 目标唯一标识
    double timestamp;

};

struct GridAndStride {
    int grid0;
    int grid1;
    int stride;
};
struct Position {
    double x;
    double y;
};
