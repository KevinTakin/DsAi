#ifndef PID_H
#define PID_H

#include <deque>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <memory>
#include <vector>
#include <optional>


class PID {
public:
    PID(double 比例系数_, double 积分系数_, double 微分系数_);

    double 控制循环(double 当前误差, double 时间间隔, double 最近目标宽高, double imgsize);
    double 获取速度() const;

    void 重置();
    void 底层参数(double 达标误差阈值_, double 速度倍率_, double 最小系数_, double 最大系数_, double 过渡锐度_, double 动态过渡中点_, double 最小数据量_, double 误差变化容限_);

    void 设置平滑因子(double α);
    // 更新PID参数
    void 更新参数(double 比例系数_, double 积分系数_, double 微分系数_);

private:
    // PID 核心参数
    double 比例系数;
    double 积分系数;
    double 微分系数;

    double 比例;
    double 积分;
    double 微分;
    double 微分输出;
    int 稳定计数 = 0;

    // 控制状态
    double 上一次误差 = 0.0;
    double 误差变化率 = 0;
    double 积分累计 = 0.0;
    // 速度控制
    double 新速度 = 0.0;
    double 当前速度 = 0.0;
    double 上一帧速度 = 0.0;
    unsigned int 加载帧数 = 0;
    double 速度倍率 = 1.0;

    // 输出平滑
    double 上一次平滑输出 = 0.0;
    double 平滑因子 = 1.0;
    double 总输出 = 0.0;

    // 状态机
    double 外环控制 = false;
    bool 已达标 = false; // 新增核心状态标志
    double 达标误差阈值 = 4; // 可配置的阈值
    double 动态判断阈值 = 0;
    double 最小系数 = 0;   // 当目标很小时(占比小)的系数
    double 最大系数 = 0;   // 当目标很大时(占比大)的系数
    double 过渡锐度 = 0;   // 控制过渡的陡峭程度
    double 动态过渡中点 = 0;
    int 最小数据量 = 2; //最少达到几个数据开始比较
    int 误差变化容限 = 2;  //误差容限，如果误差持续小于这个值，就代表陷入死区

};

#endif // PID_H
