#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <map>

// 配置管理器类，负责读取和更新配置参数
class 配置管理器 {
public:
    配置管理器(const std::string& 配置路径);
    ~配置管理器();

    // 启动后台检测线程
    void 开始监视();

    // 停止后台线程
    void 停止监视();
    void 切换监视状态(); // 新增方法：切换监视状态
    bool 正在监视() const; // 新增方法：获取当前监视状态
    // 读取参数接口
    double 获取数值(const std::string& 分组, const std::string& 名称, double 默认值);
    std::string 获取文本(const std::string& 分组, const std::string& 名称, const std::string& 默认值);
    time_t 获取版本号() const;
private:
    void 加载配置();
    void 创建默认配置();
    void 监视线程函数();
    std::atomic<bool> 监视中_; // 新增原子标志
    std::string 配置路径_;
    std::atomic<bool> 停止标志_;
    std::thread 监视线程_;

    std::map<std::string, std::map<std::string, std::string>> 当前配置_;
    time_t 最后修改时间_;
};
