#define WIN32_LEAN_AND_MEAN  // 排除不常用内容
#include <Windows.h>  
#include "参数管理器.h"
#include <iostream>


参数管理器::参数管理器(配置管理器& 配置)
    : cfg(配置), 状态_(参数状态::等待变更), 上次版本号_(0) {
}
const std::unordered_map<std::string, int> 参数管理器::键码映射 = {
    // 预定义键名
    {"VK_LBUTTON", VK_LBUTTON},
    {"VK_RBUTTON", VK_RBUTTON},
    {"VK_MBUTTON", VK_MBUTTON},
    {"VK_XBUTTON1", VK_XBUTTON1},
    {"VK_XBUTTON2", VK_XBUTTON2},
    {"VK_SHIFT", VK_SHIFT},
    {"VK_CONTROL", VK_CONTROL},
    {"VK_MENU", VK_MENU},  // Alt键
    {"VK_SPACE", VK_SPACE},
    {"VK_RETURN", VK_RETURN}, // 回车键
    {"VK_ESCAPE", VK_ESCAPE},
    {"VK_TAB", VK_TAB},
    {"VK_CAPITAL", VK_CAPITAL},  // Caps Lock
    {"VK_F1", VK_F1},
    {"VK_F2", VK_F2},
    {"VK_F3", VK_F3},
    {"VK_F4", VK_F4},
    {"VK_F5", VK_F5},
    {"VK_F6", VK_F6},
    {"VK_F7", VK_F7},
    {"VK_F8", VK_F8},
    {"VK_F9", VK_F9},
    {"VK_F10", VK_F10},
    {"VK_F11", VK_F11},
    {"VK_F12", VK_F12},

    // 字母键
    {"A", 'A'}, {"B", 'B'}, {"C", 'C'}, {"D", 'D'}, {"E", 'E'}, {"F", 'F'},
    {"G", 'G'}, {"H", 'H'}, {"I", 'I'}, {"J", 'J'}, {"K", 'K'}, {"L", 'L'},
    {"M", 'M'}, {"N", 'N'}, {"O", 'O'}, {"P", 'P'}, {"Q", 'Q'}, {"R", 'R'},
    {"S", 'S'}, {"T", 'T'}, {"U", 'U'}, {"V", 'V'}, {"W", 'W'}, {"X", 'X'},
    {"Y", 'Y'}, {"Z", 'Z'},

    // 数字键
    {"0", '0'}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'},
    {"5", '5'}, {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'},

    // 常用十六进制表示法
    {"0x01", VK_LBUTTON},
    {"0x02", VK_RBUTTON},
    {"0x04", VK_MBUTTON},
    {"0x05", VK_XBUTTON1},
    {"0x06", VK_XBUTTON2},
    {"0x10", VK_SHIFT},
    {"0x11", VK_CONTROL},
    {"0x12", VK_MENU},  // Alt键
    {"0x20", VK_SPACE},
    {"0x0D", VK_RETURN}, // 回车键
    {"0x1B", VK_ESCAPE},
    {"0x09", VK_TAB},
    {"0x14", VK_CAPITAL},  // Caps Lock
    {"0x70", VK_F1},
    {"0x71", VK_F2},
    {"0x72", VK_F3},
    {"0x73", VK_F4},
    {"0x74", VK_F5},
    {"0x75", VK_F6},
    {"0x76", VK_F7},
    {"0x77", VK_F8},
    {"0x78", VK_F9},
    {"0x79", VK_F10},
    {"0x7A", VK_F11},
    {"0x7B", VK_F12}
};

int 参数管理器::字符串到键码(const std::string& 键名) {
    // 1. 查找预定义键
    auto it = 键码映射.find(键名);
    if (it != 键码映射.end()) {
        return it->second;
    }

    // 2. 尝试解析十六进制值 (如 0x12 或 12)
    try {
        size_t pos = 0;
        int base = 10;

        // 检查是否有0x前缀
        if (键名.find("0x") == 0 || 键名.find("0X") == 0) {
            base = 16;
        }

        int 键码 = std::stoi(键名, &pos, base);

        // 确保整个字符串都被解析
        if (pos == 键名.size()) {
            return 键码;
        }
    }
    catch (...) {
        // 解析失败，继续尝试其他方法
    }

    // 3. 尝试作为单个字符解析
    if (键名.size() == 1) {
        char c = 键名[0];
        if (c >= 'A' && c <= 'Z') {
            return c; // 大写字母键
        }
        else if (c >= 'a' && c <= 'z') {
            return std::toupper(c); // 小写字母转大写
        }
        else if (c >= '0' && c <= '9') {
            return c; // 数字键
        }
    }

    // 4. 特殊键的别名
    if (键名 == "左键") return VK_LBUTTON;
    if (键名 == "右键") return VK_RBUTTON;
    if (键名 == "中键") return VK_MBUTTON;
    if (键名 == "侧键1") return VK_XBUTTON1;
    if (键名 == "侧键2") return VK_XBUTTON2;
    if (键名 == "Shift") return VK_SHIFT;
    if (键名 == "Ctrl") return VK_CONTROL;
    if (键名 == "Alt") return VK_MENU;
    if (键名 == "空格") return VK_SPACE;
    if (键名 == "回车") return VK_RETURN;
    if (键名 == "Esc") return VK_ESCAPE;
    if (键名 == "Tab") return VK_TAB;
    if (键名 == "大写锁定") return VK_CAPITAL;

    return -1; // 无效按键
}

void 参数管理器::状态轮询() {
    time_t 当前版本 = cfg.获取版本号();
    if (当前版本 != 上次版本号_) {
        状态_ = 参数状态::配置已变更;
    }
}

bool 参数管理器::是否需要更新() const {
    return 状态_ == 参数状态::配置已变更;
}

void 参数管理器::更新全部参数() {
    当前.系统 = 读取参数("系");
    当前.控制器 = 读取双轴PID("控制x", "控制y");
    当前.精调 = 读取双轴PID平滑("滑因子x", "滑因子y");
    当前.底参 = 读取双轴PID底参("底参x", "底参y");
    当前.提前 = 读取双轴提前量("提-量x", "提-量y");
   // 当前.压枪配置 = 读取压枪("压");  
    当前.热键更新 = 读取热键配置("热键");
    当前.模型名 = 读取模型配置("模设置");
    上次版本号_ = cfg.获取版本号();
    状态_ = 参数状态::参数已更新;
}

const 全部参数& 参数管理器::获取当前参数() const {
    return 当前;
}

参数 参数管理器::读取参数(const std::string& 分组名) const {
    参数 p;
    p.I.截图大小 = static_cast<int>(cfg.获取数值(分组名, "截大小", 320));
    p.I.线程数 = static_cast<int>(cfg.获取数值(分组名, "线", 4));
    p.I.yolo版本选择 = static_cast<int>(cfg.获取数值(分组名, "版", 0));
    p.I.轨迹类型 = static_cast<int>(cfg.获取数值(分组名, "轨类", 0));
    p.I.截图方式 = static_cast<int>(cfg.获取数值(分组名, "截方式", 0));
    p.I.移动方式 = static_cast<int>(cfg.获取数值(分组名, "移方式", 0));
    p.I.dt = cfg.获取数值(分组名, "dt", 0.005);
    p.I.锁定范围 = cfg.获取数值(分组名, "范", 640.0);
    p.I.上侧瞄准比例 = cfg.获取数值(分组名, "上比例", 0.1);
    p.I.下侧瞄准比例 = cfg.获取数值(分组名, "下比例", 0.1);
  //  p.I.是否压枪 = static_cast<int>(cfg.获取数值(分组名, "是否压", 1));
    p.I.置信度 = cfg.获取数值(分组名, "置", 0.6);
    p.I.预览窗口 = static_cast<bool>(cfg.获取数值(分组名, "预览", 0));
    p.I._ip = cfg.获取文本(分组名, "ip", "");   //  空字符串
    p.I._port = cfg.获取文本(分组名, "port", "15");  //  默认端口
    p.I._uuid = cfg.获取文本(分组名, "uuid", "");   //  空字符串
    p.I.name = cfg.获取文本(分组名, "name", "");   //  保持不变

   // p.I.启动扳机 = static_cast<int>(cfg.获取数值(分组名, "扳", 0));
   // p.I.最低 = static_cast<double>(cfg.获取数值(分组名, "随机下限", 0));
   // p.I.最高 = static_cast<double>(cfg.获取数值(分组名, "随机上限", 0));
    return p;
}

单轴PID 参数管理器::读取单轴PID(const std::string& 分组名) const {
    单轴PID pid;
    pid.比例 = cfg.获取数值(分组名, "例", 0.0);
    pid.积分 = cfg.获取数值(分组名, "积", 0.0);
    pid.微分 = cfg.获取数值(分组名, "微", 0.0);
    return pid;
}

双轴PID 参数管理器::读取双轴PID(const std::string& 分组X, const std::string& 分组Y) const {
    return { 读取单轴PID(分组X), 读取单轴PID(分组Y) };
}

PID平滑权重 参数管理器::读取PID平滑(const std::string& 分组名) const {
    PID平滑权重 p;
    p.输出平滑 = cfg.获取数值(分组名, "输出滑", 0.0);
    return p;
}

双轴PID平滑 参数管理器::读取双轴PID平滑(const std::string& 分组X, const std::string& 分组Y) const {
    return { 读取PID平滑(分组X), 读取PID平滑(分组Y) };
}

PID底参 参数管理器::读取PID底参(const std::string& 分组名) const {
    PID底参 p;
    p.达标误差阈值 = cfg.获取数值(分组名, "达标误差阈值", 4);
    p.速度倍率 = cfg.获取数值(分组名, "速度倍率", 1000);
    p.最小系数 = cfg.获取数值(分组名, "最小系数", 1.6);
    p.最大系数 = cfg.获取数值(分组名, "最大系数", 2.5);
    p.过渡锐度 = cfg.获取数值(分组名, "过渡锐度", 5);
    p.动态过渡中点 = cfg.获取数值(分组名, "动态过渡中点", 0.5);
    p.最小数据量 = static_cast<int>(cfg.获取数值(分组名, "最小数据量", 5));
    p.误差变化容限 = static_cast<int>(cfg.获取数值(分组名, "误差变化容限", 2));
    return p;
}

双轴PID底参 参数管理器::读取双轴PID底参(const std::string& 分组X, const std::string& 分组Y) const {
    return { 读取PID底参(分组X), 读取PID底参(分组Y) };
}

提前量 参数管理器::读取提前量(const std::string& 分组名) const {
    提前量 t;
    t.最大累计 = cfg.获取数值(分组名, "最大累计", 2);
    t.转向最小速度阈值 = cfg.获取数值(分组名, "转向最小速度阈值", 1);
    t.Y轴预瞄 = cfg.获取数值(分组名, "Y轴预", 1);
    return t;
}

双轴提前量 参数管理器::读取双轴提前量(const std::string& 分组X, const std::string& 分组Y) const {
    return { 读取提前量(分组X), 读取提前量(分组Y) };
}
/*压枪参数 参数管理器::读取压枪参数(const std::string& 分组名) const {
    压枪参数 p;
    p.压枪强度 = cfg.获取数值(分组名, "压强度", 2.8);
    p.恢复速度 = cfg.获取数值(分组名, "恢复速度", 0.8);

    p.最小准备延时 = static_cast<int>(cfg.获取数值(分组名, "最小准备延时", 10));
    p.最大准备延时 = static_cast<int>(cfg.获取数值(分组名, "最大准备延时", 20));

    p.最小压枪持续时间 = static_cast<int>(cfg.获取数值(分组名, "最小压-持续时间", 30));
    p.最大压枪持续时间 = static_cast<int>(cfg.获取数值(分组名, "最大压-持续时间", 100));

    p.最小暂停时间 = static_cast<int>(cfg.获取数值(分组名, "最小暂停时间", 20));
    p.最大暂停时间 = static_cast<int>(cfg.获取数值(分组名, "最大暂停时间", 50));

    p.单点判定阈值 = static_cast<int>(cfg.获取数值(分组名, "单点判定阈值", 70));
    p.单点专用延迟 = static_cast<int>(cfg.获取数值(分组名, "单点专用延迟", 20));

    return p;
}
压枪 参数管理器::读取压枪(const std::string& 分组名) const {
    压枪 rc;
    rc.Y = 读取压枪参数(分组名);
    return rc;
}*/
热键 参数管理器::读取热键配置(const std::string& 分组名) const {
    热键 热键配置;

    // 从配置文件读取热键设置，默认值是F1
    热键配置.T.锁定热键1 = cfg.获取文本(分组名, "锁定热键1", "F1");
    热键配置.T.锁定热键2 = cfg.获取文本(分组名, "锁定热键2", "F2");
    热键配置.T.锁定热键3 = cfg.获取文本(分组名, "锁定热键3", "F3");
    热键配置.T.锁定热键4 = cfg.获取文本(分组名, "锁定热键4", "F4");
    热键配置.T.锁定热键5 = cfg.获取文本(分组名, "锁定热键5", "F5");

    return 热键配置;
}
模型组 参数管理器::读取模型配置(const std::string& 分组名) const {
    模型组 模型配置;

    // 从配置文件读取热键设置，默认值是F1
    模型配置.x.目标模型 = cfg.获取文本(分组名, "目膜", "xxx");
    模型配置.x.路径模型 = cfg.获取文本(分组名, "路膜", "xxx");

    return 模型配置;
}
