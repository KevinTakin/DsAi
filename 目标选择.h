#pragma once
#include <vector>
#include "mokuai.h" // 包含Aim结构体定义

struct 目标选择结果 {
    double 横向误差;
    double 纵向误差;
    double 目标宽度;
    double 目标高度;
    double 目标位置x;
    double 目标位置y;
    Aim 选中目标;
    bool 是锁定目标;
};

目标选择结果 选择目标(const std::vector<Aim>& 有效目标, double offset_x, double offset_y);