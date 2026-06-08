    #include "配置管理器.h"
    #include "ini库.h"
    #include <fstream>
    #include <iostream>
    #include <filesystem>
    #include <chrono>
    #include <thread>
    #include <Windows.h>  // 必须包含 Windows 头文件

    配置管理器::配置管理器(const std::string& 配置路径)
        : 配置路径_(配置路径), 停止标志_(false), 监视中_(false), 最后修改时间_(0) {
        if (!std::filesystem::exists(配置路径_)) {
            创建默认配置();
        }
        加载配置();
    }

    配置管理器::~配置管理器() {
        停止监视();
    }
    // 在配置管理器实现文件中添加
    void 配置管理器::切换监视状态() {
        if (监视中_) {
            停止监视();
            Beep(600, 300);  // 低音提示
        }
        else {
            开始监视();
            Beep(800, 200);  // 高音提示
        }
    }

    bool 配置管理器::正在监视() const {
        return 监视中_;
    }

    void 配置管理器::开始监视() {
        停止标志_ = false;
        监视中_ = true; // 设置状态标志
        监视线程_ = std::thread(&配置管理器::监视线程函数, this);
    }

    void 配置管理器::停止监视() {
        停止标志_ = true;
        if (监视线程_.joinable()) {
            监视线程_.join();
        }
    }

    void 配置管理器::创建默认配置() {
        std::ofstream 文件(配置路径_);
        文件 << "#按下 END 键可开关实时更新，调整完参数记得关闭,程序启动时实时调参默认是关闭的！\n";
        文件 << "#按下键盘 左 右 方向键可调整锁定阵营。同时长按 左 右 方向键1.5秒后关闭阵营选择，启动全局。再次长按可返回阵营模式\n";
        文件 << "#按下键盘 上 下 方向键可调整锁定位置\n";
        文件 << "[模设置]\n";
        文件 << "目膜=222\n";
        文件 << "路膜=111\n\n";

        文件 << "[系]\n";
        文件 << "截大小=320\n";
        文件 << "线=4\n";
        文件 << "版=0\n";
        文件 << "轨类=0\n";
        文件 << "截方式=0\n";
        文件 << "移方式=0\n";
        文件 << "#以上设置需要重新启动程序!\n\n";

        文件 << "#以下设置可实时修改!\n";
        文件 << "dt=0.001\n";
        文件 << "范=640\n";
        文件 << "上比例=0.9\n";
        文件 << "下比例=0.1\n";
        文件 << "置=0.6\n";
        文件 << "预览=0\n";
        文件 << "ip=\n";           // 留空或填默认值
        文件 << "port=15\n";
        文件 << "uuid=\n";
        文件 << "name=\n\n";


        文件 << "[控制x]\n";
        文件 << "例=0.3\n";
        文件 << "积=8\n";
        文件 << "微=0.003\n\n";

        文件 << "[控制y]\n";
        文件 << "例=0.3\n";
        文件 << "积=8\n";
        文件 << "微=0.003\n\n";

        文件 << "[滑因子x]\n";
        文件 << "输出滑=0.4\n\n";

        文件 << "[滑因子y]\n";
        文件 << "输出滑=0.4\n\n";

        文件 << "[底参x]\n";
        文件 << "达标误差阈值=1\n";
        文件 << "速度倍率=1000\n";
        文件 << "最小系数=1.6\n";
        文件 << "最大系数=2.5\n";
        文件 << "过渡锐度=5\n";
        文件 << "动态过渡中点=0.5\n";
        文件 << "最小数据量=5\n";
        文件 << "误差变化容限=2\n\n";

        文件 << "[底参y]\n";
        文件 << "达标误差阈值=1\n";
        文件 << "速度倍率=1000\n";
        文件 << "最小系数=1.6\n";
        文件 << "最大系数=2.5\n";
        文件 << "过渡锐度=5\n";
        文件 << "动态过渡中点=0.5\n";
        文件 << "最小数据量=5\n";
        文件 << "误差变化容限=2\n\n";

        文件 << "[提-量x]\n";
        文件 << "最大累计=2\n";
        文件 << "转向最小速度阈值=1\n";

        文件 << "[提-量y]\n";
        文件 << "最大累计=2\n";
        文件 << "转向最小速度阈值=1\n";
        文件 << "Y轴预=1\n\n";

        文件 << "#快捷键修改需要重启后生效\n";
        文件 << "[热键]\n";
        文件 << "锁定热键1=VK_RBUTTON\n";
        文件 << "锁定热键2=VK_SHIFT\n";
        文件 << "锁定热键3=F1\n";
        文件 << "锁定热键4=F1\n";
        文件 << "锁定热键5=F1\n";
        文件.close();
    }

    void 配置管理器::加载配置() {
        INIReader 读取器(配置路径_);
        if (读取器.ParseError() != 0) {
            std::cerr << "配置文件加载失败：" << 配置路径_ << std::endl;
            return;
        }

        当前配置_.clear();
        for (const auto& 分组 : 读取器.Sections()) {
            for (const auto& 键 : 读取器.Keys(分组)) {
                当前配置_[分组][键] = 读取器.Get(分组, 键, "");
            }
        }

        最后修改时间_ = std::filesystem::last_write_time(配置路径_).time_since_epoch().count();
    }

    void 配置管理器::监视线程函数() {
        while (!停止标志_) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);

            try {
                // 检查文件是否存在
                if (!std::filesystem::exists(配置路径_)) {
                    std::cerr << "配置文件不存在，等待重新出现..." << std::endl;
                    continue; // 跳过本次检查，等待下次循环
                }

                // 获取文件修改时间（需要额外捕获异常）
                auto 当前时间 = std::filesystem::last_write_time(配置路径_).time_since_epoch().count();
                if (当前时间 != 最后修改时间_) {
                    try {
                        std::cout << "配置文件发生变化，重新加载...\n";
                        加载配置();
                    }
                    catch (const std::exception& e) {
                        std::cerr << "配置文件加载失败: " << e.what() << std::endl;
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "文件系统错误: " << e.what() << std::endl;
            }
        }
        监视中_ = false;
    }


    double 配置管理器::获取数值(const std::string& 分组, const std::string& 名称, double 默认值) {
        try {
            return std::stod(当前配置_[分组][名称]);
        }
        catch (...) {
            return 默认值;
        }
    }

    std::string 配置管理器::获取文本(const std::string& 分组, const std::string& 名称, const std::string& 默认值) {
        if (当前配置_.count(分组) && 当前配置_[分组].count(名称)) {
            return 当前配置_[分组][名称];
        }
        return 默认值;
    }
    time_t 配置管理器::获取版本号() const {
        return 最后修改时间_;
    }
