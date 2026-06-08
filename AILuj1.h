#ifndef MOUSEPREDICTOR_H
#define MOUSEPREDICTOR_H

#include <ncnn/net.h>
#include <vector>
#include <utility>

class AILuj {
public:
    // 构造函数，加载ncnn模型
    void init(const char* param_path, const char* bin_path);

    // 预测路径的函数，输入为目标坐标（x, y）
    std::vector<std::pair<double, double>> predict(double x, double y);

private:
    ncnn::Option opt;
    ncnn::Net net;
    ncnn::Mat input;
    std::vector<double> prev_coord;
};



#endif // MOUSEPREDICTOR_H
