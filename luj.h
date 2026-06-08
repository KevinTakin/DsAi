#ifndef LUJ_H
#define LUJ_H

#include "AILuj1.h"
#include "贝塞尔随机.h" // 包含贝塞尔生成器的头文件
#include <vector>
#include <utility>

class LUJ
{
public:
    void init(const char* param_path, const char* bin_path, int modes);
    std::vector<std::pair<double, double>> predict(double x, double y);

private:
    int mode;
    AILuj ailuj;
    BezierMovementGenerator bezierGenerator; // 添加贝塞尔生成器实例
};

#endif