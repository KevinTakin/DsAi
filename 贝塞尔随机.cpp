#include "贝塞尔随机.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <numbers>

BezierMovementGenerator::BezierMovementGenerator() {
    random_engine.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

double BezierMovementGenerator::randomOffset(double scale) {
    std::uniform_real_distribution<double> distribution(-scale, scale);
    return distribution(random_engine);
}

double BezierMovementGenerator::randomJitter(double scale) {
    std::uniform_real_distribution<double> distribution(-scale, scale);
    return distribution(random_engine);
}

void BezierMovementGenerator::reset() {
    isResetMode = false; // 重置为正常模式
}

std::vector<std::pair<double, double>> BezierMovementGenerator::predict(double dx, double dy) {
    if (dx == 0 && dy == 0) {
        return std::vector<std::pair<double, double>>();
    }

    // 计算移动距离和方向
    double dist = std::sqrt(dx * dx + dy * dy);

    // 修正逻辑：根据当前状态选择合适的阈值
    if (!isResetMode && dist < 5) {  // 正常模式，距离<5
        std::vector<std::pair<double, double>> result;
        result.push_back(std::make_pair(dx, dy));
        isResetMode = true; // 进入重置模式
        return result;
    }

    if (isResetMode && dist < 30) {  // 重置模式，距离<30
        std::vector<std::pair<double, double>> result;
        result.push_back(std::make_pair(dx, dy));
        return result;
    }

    // 如果距离>=30，则无论什么模式都生成曲线
    isResetMode = false; // 距离>=30时自动退出重置模式

    double angle = std::atan2(dy, dx);

    // 根据距离确定采样点数量
    int num_samples = static_cast<int>(std::max(5.0, std::min(15.0, std::ceil(dist / 5))));

    // 定义贝塞尔曲线的起点和终点
    double p0_x = 0.0, p0_y = 0.0;
    double p2_x = dx, p2_y = dy;

    // 计算中点
    double mid_x = (p0_x + p2_x) / 2.0;
    double mid_y = (p0_y + p2_y) / 2.0;

    // 创建更明显的曲线效果
    // 使用标准方法获取 π 值
#if __cplusplus >= 202002L
    // C++20 及以上版本使用 std::numbers::pi
    constexpr double pi = std::numbers::pi;
#else
    // 对于早期版本，手动定义 π
    constexpr double pi = 3.14159265358979323846;
#endif

    double offset_dist = dist * 0.1; // 增加曲线幅度
    double perp_angle = angle + pi / 2; // 垂直方向

    // 随机选择曲线方向（上或下）
    double curve_direction = (randomOffset(1.0) > 0) ? 1.0 : -1.0;

    // 计算控制点位置
    double p1_x = mid_x + curve_direction * offset_dist * std::cos(perp_angle);
    double p1_y = mid_y + curve_direction * offset_dist * std::sin(perp_angle);

    // 添加额外随机偏移
    p1_x += randomOffset(dist * 0.1);
    p1_y += randomOffset(dist * 0.1);

    // 采样贝塞尔曲线上的点
    std::vector<std::pair<double, double>> sampled_points;
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / (num_samples - 1);
        double x = (1 - t) * (1 - t) * p0_x + 2 * (1 - t) * t * p1_x + t * t * p2_x;
        double y = (1 - t) * (1 - t) * p0_y + 2 * (1 - t) * t * p1_y + t * t * p2_y;

        // 添加微小抖动
        double jitter_scale = 0.5; // 抖动幅度
        x += randomJitter(jitter_scale);
        y += randomJitter(jitter_scale);

        sampled_points.push_back(std::make_pair(x, y));
    }

    // 转换为相对移动
    std::vector<std::pair<double, double>> relative_moves;
    for (size_t i = 1; i < sampled_points.size(); ++i) {
        double delta_x = sampled_points[i].first - sampled_points[i - 1].first;
        double delta_y = sampled_points[i].second - sampled_points[i - 1].second;
        relative_moves.push_back(std::make_pair(delta_x, delta_y));
    }

    return relative_moves;
}
