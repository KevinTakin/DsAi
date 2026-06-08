#pragma once
#include "mokuai.h"  // Aim结构定义
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>

class 简易目标筛选器 {
public:
    简易目标筛选器(const std::string&);

    void 从配置文件加载类别(std::string path);
    void 处理快捷键();
    int 获取当前选中类别() const;
    std::string 获取当前类别名称() const;
    std::vector<Aim> 筛选(const std::vector<Aim>& 所有目标) const;  // 移除了锁定目标ID参数

    // 新增方法
    const std::vector<int>& 获取配置类别列表() const {
        return 配置类别列表;
    }
    bool 是否筛选全部() const {
        return 当前索引 == -1;
    }

private:
    void 更新类别索引(int 新索引);

    mutable std::mutex mtx;
    int 当前索引; // -1表示所有配置类别，0及以上表示具体类别索引
    std::vector<int> 配置类别列表; // 配置文件中的类别
    bool 全局模式 = false;
};
