#ifndef 提前量控制器_H
#define 提前量控制器_H

struct 偏移状态 {
    double 上一帧速度x = 0.0;
    double 上一帧速度y = 0.0;
    double 当前累计x = 0.0;
    double 当前累计y = 0.0;
    bool 锁定x = false;
    bool 锁定y = false;
};

class 提前量控制器 {
public:
    // 设置参数接口
    void 设置提前(
        double 最大累计y,
        double 转向最小速度阈值_,
        bool Y轴预瞄_
    );

    // 原函数封装（完全保持参数不变）
    void 更新偏移量(
        double& offset_x,
        double& offset_y,
        const double offset,
        const double dt,
        const double 速度x,
        const double 速度y,
        偏移状态& 状态
    ) const;

private:
    double 最大累计x = 2.0;
    double 最大累计y = 2.0;
    double 转向最小速度阈值 = 1.0;
    bool Y轴预瞄 = false;
};

#endif // 提前量控制器_H
