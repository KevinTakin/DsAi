#include "提前量.h"
#include <cmath>

void 提前量控制器::设置提前(
    double 最大累计,
    double 转向最小速度阈值_,
    bool Y轴预瞄_
) {
    最大累计x = 最大累计;
    最大累计y = 最大累计;
    转向最小速度阈值 = 转向最小速度阈值_;
    Y轴预瞄 = Y轴预瞄_;
}

void 提前量控制器::更新偏移量(
    double& offset_x,
    double& offset_y,
    const double offset,
    const double dt,
    const double 速度x,
    const double 速度y,
    偏移状态& 状态
) const {
    bool 转向x = (速度x * 状态.上一帧速度x < 0) && (std::abs(速度x) > 转向最小速度阈值);
    bool 转向y = (速度y * 状态.上一帧速度y < 0) && (std::abs(速度y) > 转向最小速度阈值);

    // X轴处理（原样保留）
    if (转向x) {
        状态.当前累计x = 0.0;
        offset_x = offset;
        状态.锁定x = false;
    }
    else if (!状态.锁定x) {
        double 增量x = 速度x * dt;
        状态.当前累计x += 增量x;

        if (状态.当前累计x >= 最大累计x) {
            状态.当前累计x = 最大累计x;
            状态.锁定x = true;
        }
        else if (状态.当前累计x <= -最大累计x) {
            状态.当前累计x = -最大累计x;
            状态.锁定x = true;
        }

        offset_x += 增量x;
    }

    // Y轴处理（原样保留）
    if (Y轴预瞄) {
        状态.当前累计y = 0.0;
        offset_y = offset;
        状态.锁定y = false;
    }
    else if (转向y) {
        状态.当前累计y = 0.0;
        offset_y = offset;
        状态.锁定y = false;
    }
    else if (!状态.锁定y) {
        double 增量y = 速度y * dt;
        状态.当前累计y += 增量y;

        if (状态.当前累计y >= 最大累计y) {
            状态.当前累计y = 最大累计y;
            状态.锁定y = true;
        }
        else if (状态.当前累计y <= -最大累计y) {
            状态.当前累计y = -最大累计y;
            状态.锁定y = true;
        }

        offset_y += 增量y;
    }

    状态.上一帧速度x = 速度x;
    状态.上一帧速度y = 速度y;
}
