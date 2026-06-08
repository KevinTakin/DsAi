#pragma once
#include <string>

// 修复模型名称结构体 - 确保成员变量名一致
struct 模型名称 {
    std::string 目标模型;
    std::string 路径模型;  // 修改为路径模型以匹配读取方法

};

struct 模型组 {
    模型名称 x;
};
struct 单轴PID {
    double 比例 = 0;
    double 积分 = 0;
    double 微分 = 0;
};

struct 双轴PID {
    单轴PID X;
    单轴PID Y;
};

// 你可以再定义其他结构体，比如pid精调、热键等
struct PID平滑权重 {

    double 输出平滑 = 0;
};
struct 双轴PID平滑 {
    PID平滑权重 X;
    PID平滑权重 Y;
};

struct PID底参 {
    double 达标误差阈值 = 0;
    double 速度倍率 = 0;
    double 最小系数 = 0;  
    double 最大系数 = 0;  
    double 过渡锐度 = 0;  
    double 动态过渡中点 = 0;
    int 最小数据量 = 5; //最少达到几个数据开始比较
    int 误差变化容限 = 2;  //误差容限，如果误差持续小于这个值，就代表陷入死区
};
struct 双轴PID底参 {
    PID底参 X;
    PID底参 Y;
};

struct 提前量 {
    double 最大累计 = 0;
    double 转向最小速度阈值 = 0;
    double Y轴预瞄 = 0;

};
struct 双轴提前量 {
    提前量 X;
    提前量 Y;
};

struct 系统参数 {
    int 截图大小 = 320;
    int 线程数 = 4; // 线程
    int yolo版本选择 = 0;
    int 轨迹类型 = 0;
    int 截图方式 = 0;
    int 移动方式 = 0;
    double dt = 0;
    double 锁定范围 = 640;
    double 上侧瞄准比例 = 0.8;  // 可调整百分比
    double 下侧瞄准比例 = 0.5;  // 可调整百分比
    int 是否压枪 = 1;
    double 置信度 = 0.6;  // 置信度
    bool 预览窗口 = 0;
    int 启动扳机 = 0;
    double 最低 = 0;
    double 最高 = 0;
    std::string _ip;
    std::string _port;
    std::string _uuid;
    std::string name;

};
struct 参数 {
    系统参数 I;
};

struct 压枪参数 {
       
    double 压枪强度 = 0;        // recoil_factor：每帧压枪补偿量
    double 恢复速度 = 0;        // recovery_factor：未射击时误差恢复比例

    int 最小准备延时 = 0;       // minDelay：开火后延迟开始压枪（ms）
    int 最大准备延时 = 0;       // maxDelay

    int 最小压枪持续时间 = 0;   // minDuration：单次压枪持续时间（ms）
    int 最大压枪持续时间 = 0;  // maxDuration

    int 最小暂停时间 = 0;       // minPause：压完暂停时间（ms）
    int 最大暂停时间 = 0;       // maxPause

    int 单点判定阈值 = 0;       // singleShotThreshold：按压小于此时间判定为单点（ms）
    int 单点专用延迟 = 0;       // singleShotDelay：单点状态延迟压枪时间（ms）
};
struct 压枪 {
    压枪参数 Y;
};
struct 热键设置
{
    // 添加快捷键配置
    std::string 锁定热键1;
    std::string 锁定热键2;
    std::string 锁定热键3;
    std::string 锁定热键4;
    std::string 锁定热键5;
};
struct 热键 {
    热键设置 T;
};