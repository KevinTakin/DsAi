#pragma once
#include "配置管理器.h"
#include "参数结构体.h"
#include <string>
#include <unordered_map>

// 参数状态定义
enum class 参数状态 {
    等待变更,      // 当前是最新参数，无需更新
    配置已变更,    // 检测到配置变化，需要重新读取参数
    参数已更新     // 已成功更新参数
};
struct 全部参数 {
    参数 系统;                  // 系统基础参数
    双轴PID 控制器;             // PID 控制器
    双轴PID平滑 精调;          // PID 平滑参数
    双轴PID底参 底参;          // 控制底参
    双轴提前量 提前;           // 提前量配置
  //  压枪 压枪配置;  // 添加压枪配置
    热键 热键更新;
    模型组 模型名;
};
class 参数管理器 {
public:
    参数管理器(配置管理器& 配置);

    void 状态轮询();
    bool 是否需要更新() const;
    void 更新全部参数();
    static int 字符串到键码(const std::string& 键名);
    const 全部参数& 获取当前参数() const;

private:
    配置管理器& cfg;
    参数状态 状态_;
    time_t 上次版本号_;
    全部参数 当前;

    参数 读取参数(const std::string& 分组名) const;
    单轴PID 读取单轴PID(const std::string& 分组名) const;
    双轴PID 读取双轴PID(const std::string& 分组X, const std::string& 分组Y) const;
    PID平滑权重 读取PID平滑(const std::string& 分组名) const;
    双轴PID平滑 读取双轴PID平滑(const std::string& 分组X, const std::string& 分组Y) const;
    PID底参 读取PID底参(const std::string& 分组名) const;
    双轴PID底参 读取双轴PID底参(const std::string& 分组X, const std::string& 分组Y) const;
    提前量 读取提前量(const std::string& 分组名) const;
    双轴提前量 读取双轴提前量(const std::string& 分组X, const std::string& 分组Y) const;
    压枪参数 读取压枪参数(const std::string& 分组名) const;
    压枪 读取压枪(const std::string& 分组名) const;
    static const std::unordered_map<std::string, int> 键码映射;
    热键 读取热键配置(const std::string& 分组名) const;
    模型组 读取模型配置(const std::string& 分组名) const;
};
