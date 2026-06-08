#ifndef BEZIER_RANDOM_H
#define BEZIER_RANDOM_H

#include <vector>
#include <utility>
#include <random>
#include <cmath>

class BezierMovementGenerator {
public:
    BezierMovementGenerator();
    std::vector<std::pair<double, double>> predict(double dx, double dy);
    void reset(); // 重置函数

private:
    bool isResetMode = false; // 使用成员初始化
    std::default_random_engine random_engine;
    double randomOffset(double scale);
    double randomJitter(double scale);
};

#endif // BEZIER_RANDOM_H
