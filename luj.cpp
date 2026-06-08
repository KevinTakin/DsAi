#include "LUJ.h"
#include <iostream>

void LUJ::init(const char* param_path, const char* bin_path, int modes) {
    mode = modes;
    if (modes == 0) {
        ailuj.init(param_path, bin_path);
    }

    // 模式1（贝塞尔）不需要特别的初始化
}

std::vector<std::pair<double, double>> LUJ::predict(double x, double y) {
    if (mode == 0) {
        return ailuj.predict(x, y);
    }
    else if (mode == 1) {
        // 使用贝塞尔曲线生成移动轨迹
        return bezierGenerator.predict(x, y);
    }

    // 默认返回空向量
    return std::vector<std::pair<double, double>>();
}