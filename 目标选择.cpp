#include "目标选择.h"
#include <algorithm>
#include <cmath>
#include <limits> // 用于 std::numeric_limits

目标选择结果 选择目标(const std::vector<Aim>& 有效目标, double offset_x, double offset_y) {
    目标选择结果 结果{};
    if (有效目标.empty()) {
        // 返回默认空结果
        return 结果;
    }

    double 最小距离 = std::numeric_limits<double>::max();
    Aim 最佳目标 = 有效目标[0]; ; // 保持“最佳目标”的命名，但基于距离选择

    for (const auto& aim : 有效目标) {
        // 计算目标中心点到 (offset_x, offset_y) 的欧几里得距离
        double dx = aim.centerX - offset_x;
        double dy = aim.centerY - offset_y; // 修复：使用 centerY 而不是 centerX
        double 距离 = std::sqrt(dx * dx + dy * dy);

        // 更新最小距离和最佳目标
        if (距离 < 最小距离) {
            最小距离 = 距离;
            最佳目标 = aim;
        }
    }

    // 返回结果：目标属性使用原始值，误差使用预测位置计算
    结果.选中目标 = 最佳目标;
    结果.横向误差 = 最佳目标.centerX - offset_x;
    结果.纵向误差 = 最佳目标.centerY - offset_y;
    结果.目标宽度 = 最佳目标.width;
    结果.目标高度 = 最佳目标.height;
    结果.目标位置x = 最佳目标.centerX;
    结果.目标位置y = 最佳目标.centerY;
    结果.是锁定目标 = true;  // 设置锁定标志

    return 结果;
}
