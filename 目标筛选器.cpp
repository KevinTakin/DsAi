#include "目标筛选器.h"
#include <Windows.h>
#include <unordered_set>
#include <chrono>
#include <algorithm>

简易目标筛选器::简易目标筛选器(const std::string& path) : 当前索引(0), 全局模式(false) {
    从配置文件加载类别(path);
}

void 简易目标筛选器::从配置文件加载类别(std::string path) {
    std::lock_guard<std::mutex> lock(mtx);
    配置类别列表.clear();

    // 读取配置文件
    std::ifstream 文件(path);
    if (文件.is_open()) {
        std::unordered_set<int> 临时集合;
        int id;
        while (文件 >> id) {
            // 简单去重
            if (临时集合.find(id) == 临时集合.end()) {
                临时集合.insert(id);
                配置类别列表.push_back(id);
            }
        }
        文件.close();
    }

    // 如果没有配置任何类别，添加默认值
    if (配置类别列表.empty()) {
        配置类别列表 = { 0, 1 };
    }

    // 确保当前索引有效
    if (当前索引 >= static_cast<int>(配置类别列表.size())) {
        当前索引 = 0;
    }
}

void 简易目标筛选器::更新类别索引(int 新索引) {
    std::lock_guard<std::mutex> lock(mtx);
    int 类别数量 = static_cast<int>(配置类别列表.size());

    if (类别数量 == 0) {
        当前索引 = 0;
        return;
    }

    // 全局模式（-1）不需要轮换
    if (全局模式) {
        return;
    }

    // 修改为边界限制模式（不再循环）
    // 确保新索引在有效范围内 [0, 类别数量-1]
    当前索引 = (std::max)(0, (std::min)(新索引, 类别数量 - 1));
}

void 简易目标筛选器::处理快捷键() {
    // 获取当前按键状态
    bool 左键按下 = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
    bool 右键按下 = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;

    // 静态变量用于跟踪按键状态
    static bool 左键上次状态 = false;
    static bool 右键上次状态 = false;
    static auto 长按开始时间 = std::chrono::steady_clock::now();
    static bool 正在检测长按 = false;
    static auto 最后按键时间 = std::chrono::steady_clock::now();

    auto 当前时间 = std::chrono::steady_clock::now();
    auto 时间间隔 = std::chrono::duration_cast<std::chrono::milliseconds>(当前时间 - 最后按键时间).count();

    // 1. 检测左右键同时长按1.5秒
    if (左键按下 && 右键按下) {
        if (!正在检测长按) {
            长按开始时间 = 当前时间;
            正在检测长按 = true;
        }
        else {
            auto 持续时间 = std::chrono::duration_cast<std::chrono::milliseconds>(当前时间 - 长按开始时间);

            if (持续时间.count() >= 1500) {
                std::lock_guard<std::mutex> lock(mtx);
                全局模式 = !全局模式;  // 切换模式

                // 播放提示音
                if (全局模式) {
                    Beep(500, 300);  // 中音
                    Beep(800, 300);  // 高音（全局模式）
                }
                else {
                    Beep(500, 300);  // 中音
                    Beep(200, 300);  // 低音（列表模式）
                }

                正在检测长按 = false;
                最后按键时间 = 当前时间;
            }
        }
    }
    else {
        正在检测长按 = false;
    }

    // 2. 处理单独按键（在列表模式下才响应）
    if (时间间隔 > 100 && !全局模式) { // 100ms冷却时间防止抖动
        bool 左键新按下 = (左键按下 && !左键上次状态);
        bool 右键新按下 = (右键按下 && !右键上次状态);

        if (左键新按下) {
            // 左键按下：索引减1，但最小为0
            更新类别索引(当前索引 - 1);
            Beep(200, 200);  // 低音提示
            最后按键时间 = 当前时间;
        }
        else if (右键新按下) {
            // 右键按下：索引加1，但最大为类别数量-1
            更新类别索引(当前索引 + 1);
            Beep(800, 200);  // 高音提示
            最后按键时间 = 当前时间;
        }
    }

    // 更新上次按键状态
    左键上次状态 = 左键按下;
    右键上次状态 = 右键按下;
}

int 简易目标筛选器::获取当前选中类别() const {
    std::lock_guard<std::mutex> lock(mtx);
    if (全局模式) {
        return -1; // 全局模式返回-1
    }

    if (当前索引 < 0 || 当前索引 >= static_cast<int>(配置类别列表.size())) {
        return 配置类别列表.empty() ? -1 : 配置类别列表[0];
    }
    return 配置类别列表[当前索引];
}

std::string 简易目标筛选器::获取当前类别名称() const {
    std::lock_guard<std::mutex> lock(mtx);
    if (全局模式) {
        return "所有配置类别";
    }

    if (配置类别列表.empty()) {
        return "无可用类别";
    }

    if (当前索引 < 0 || 当前索引 >= static_cast<int>(配置类别列表.size())) {
        return "类别" + std::to_string(配置类别列表[0]);
    }

    return "类别" + std::to_string(配置类别列表[当前索引]);
}

std::vector<Aim> 简易目标筛选器::筛选(const std::vector<Aim>& 所有目标) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<Aim> 结果;

    if (配置类别列表.empty()) {
        return 结果;
    }

    // 创建有效类别集合
    std::unordered_set<int> 有效类别集合(配置类别列表.begin(), 配置类别列表.end());

    // 全局模式：返回所有有效类别目标
    if (全局模式) {
        for (const auto& aim : 所有目标) {
            if (有效类别集合.find(aim.label) != 有效类别集合.end()) {
                结果.push_back(aim);
            }
        }
        return 结果;
    }

    // 列表模式：返回当前类别目标
    if (当前索引 >= 0 && 当前索引 < static_cast<int>(配置类别列表.size())) {
        int 当前类别 = 配置类别列表[当前索引];
        for (const auto& aim : 所有目标) {
            if (aim.label == 当前类别) {
                结果.push_back(aim);
            }
        }
    }

    return 结果;
}
