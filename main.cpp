#include "mobile.h"
#include "cap.h"
#include "yolo.h"
#include "PID.h"
#include "提前量.h"
#include "luj.h"
#include "目标选择.h"
#include "目标筛选器.h"
#include "参数管理器.h"
#include "配置管理器.h"
#include "kaerman.hpp"
#include "日志.h"
/*免责声明 (Disclaimer)
本项目（包括但不限于源代码、文档、相关模型等）的开源仅为了促进计算机视觉、人工智能及C++编程技术的学习与交流。为了明确使用边界及规避相关风险，特此声明：

1. 仅限学习与交流用途
本项目的源代码仅供开发者进行技术学习、研究和代码交流使用。旨在探讨AI技术在图像识别、目标检测及自动化处理领域的应用原理。严禁将本项目用于任何破坏游戏公平性、违反游戏/软件服务条款（ToS）或任何非法用途。

2. 严禁商业用途
本项目的所有源代码及打包编译后的成品软件程序，绝对禁止用于任何形式的商业用途或盈利活动。任何人不得将本项目二次开发后作为商业外挂、辅助工具进行售卖或分发。

3. 遵守法律法规与用户协议
使用者在下载、编译或运行本项目时，必须严格遵守当地法律法规以及相关游戏/软件的用户协议。请勿将本项目应用于任何未经授权的真实游戏环境或多人在线竞技平台中。

4. 风险自担与免责条款
本项目按“原样”提供，作者不对代码的稳定性、安全性或适用性做任何明示或暗示的保证。
因下载、编译、修改、传播或使用本项目所产生的任何直接或间接后果（包括但不限于游戏账号被封禁、设备损坏、数据丢失、法律纠纷等），均由使用者本人自行承担。项目作者及贡献者不承担任何由此引发的法律责任及连带责任。

5. 最终解释权
一旦您下载、查看或使用本项目，即表示您已阅读、理解并同意接受本免责声明的全部条款。作者保留随时修改、更新本声明或终止该开源项目的权利。若发现有严重违反本声明的行为，作者有权要求相关方立即停止违规行为，并保留追究其责任的权利。*/
std::string generate_random_name(int length) {
    // 定义字符集（字母和数字）
    const std::string charset =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    // 设置随机数生成器
    std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);

    std::string result;
    result.reserve(length);

    // 生成随机字符串
    for (int i = 0; i < length; ++i) {
        result += charset[dist(rng)];
    }

    return result;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::string logPath = "log.qp";
    if (!Logger::Initialize(logPath)) {
        MessageBoxA(NULL, "无法创建日志文件", "错误", MB_ICONERROR);
        return -1;
    }
    std::string ReadLicenseKey(const std::string & path);
    Beep(500, 200);  // 高音提示
    std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> len_dist(8, 12);
    std::string 窗口名 = generate_random_name(len_dist(rng));
    /*AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);*/
    std::cout << "启动成功。" << std::endl;

  
    CAP cap;
    YOLO yolo;
    LUJ luj;
    Mobile mobile;
    Mobile mmove;
    PID pid_x(0.35, 6, 0.004);   // 横向PID
    PID pid_y(0.35, 6, 0.004);  // 纵向PID
    MultiTracker tracker;
    提前量控制器 控制器;
    控制器.设置提前(3.0, 0.8, true); // 设置参数
    double 最大累计x = 2;
    double 最大累计y = 2;
    double 转向最小速度阈值 = 1;
    bool Y轴预瞄 = 1;//0为关闭
    偏移状态 状态;
  //  RecoilController y压枪;
    std::string 配置路径 = ("idc.cd");
    简易目标筛选器 目标筛选器(配置路径);
    配置管理器 配置("config.ini");
    参数管理器 参数(配置);
    全部参数 当前参数;
    配置.开始监视();  // 启动后台线程实时更新配置
    static bool lastFireState = false;
    double currentErrorY = 0;
    int 锁定目标ID = -1; // 初始化为未锁定状态，放在循环外部
    double dt = 0.005;

    // 目标相关
    int 截图大小 = 320;
    int offset = 截图大小 / 2;
    double offset_x = offset;
    double offset_y = offset;
    double 置信度 = 0.6;  // 置信度
    int 线程数 = 4; // 线程
    int yolo版本选择 = 0;
    int 轨迹类型 = 0;
    int 截图方式 = 0;
    int 移动方式 = 1;
    double 锁定范围 = 640;
    double 上侧瞄准比例 = 0.8;  // 可调整百分比
    double 下侧瞄准比例 = 0.5;  // 可调整百分比
    bool 使用上侧瞄准比例 = true; // 默认使用上侧比例
 //   int 是否压枪 = 1;
    bool 预览窗口 = 1;
  //  int 启动扳机 = 0;
    double 最低 = 0;
    double 最高 = 0;
    参数.状态轮询();
    参数.更新全部参数();

    // 读取最新参数（一次性结构体打包好）
    当前参数 = 参数.获取当前参数();
    // 解包赋值（只在ini变动后执行一次）
    截图大小 = 当前参数.系统.I.截图大小;
    线程数 = 当前参数.系统.I.线程数;
    yolo版本选择 = 当前参数.系统.I.yolo版本选择;
    轨迹类型 = 当前参数.系统.I.轨迹类型;
    截图方式 = 当前参数.系统.I.截图方式;
    移动方式 = 当前参数.系统.I.移动方式;
    dt = 当前参数.系统.I.dt;
    锁定范围 = 当前参数.系统.I.锁定范围;
    上侧瞄准比例 = 当前参数.系统.I.上侧瞄准比例;
    下侧瞄准比例 = 当前参数.系统.I.下侧瞄准比例;
   // 是否压枪 = 当前参数.系统.I.是否压枪;
    置信度 = 当前参数.系统.I.置信度;
    预览窗口 = 当前参数.系统.I.预览窗口;
  //  启动扳机 = 当前参数.系统.I.启动扳机;
    最低 = 当前参数.系统.I.最低;
    最高 = 当前参数.系统.I.最高;
   // mmove.设置随机射速范围(当前参数.系统.I.最低, 当前参数.系统.I.最高);

    // 更新 PID 控制器参数
    pid_x.更新参数(当前参数.控制器.X.比例,
        当前参数.控制器.X.积分,
        当前参数.控制器.X.微分);

    pid_y.更新参数(当前参数.控制器.Y.比例,
        当前参数.控制器.Y.积分,
        当前参数.控制器.Y.微分);

    // 更新 PID 平滑因子
    pid_x.设置平滑因子(当前参数.精调.X.输出平滑);
    pid_y.设置平滑因子(当前参数.精调.Y.输出平滑);

    // 更新 PID 底层参数
    pid_x.底层参数(
        当前参数.底参.X.达标误差阈值,
        当前参数.底参.X.速度倍率,
        当前参数.底参.X.最小系数,
        当前参数.底参.X.最大系数,
        当前参数.底参.X.过渡锐度,
        当前参数.底参.X.动态过渡中点,
        当前参数.底参.X.最小数据量,
        当前参数.底参.X.误差变化容限
    );

    pid_y.底层参数(
        当前参数.底参.Y.达标误差阈值,
        当前参数.底参.Y.速度倍率,
        当前参数.底参.Y.最小系数,
        当前参数.底参.Y.最大系数,
        当前参数.底参.Y.过渡锐度,
        当前参数.底参.Y.动态过渡中点,
        当前参数.底参.Y.最小数据量,
        当前参数.底参.Y.误差变化容限
    );

    // 提前量设置
    控制器.设置提前(
        当前参数.提前.X.最大累计,
        当前参数.提前.X.转向最小速度阈值,
        当前参数.提前.X.Y轴预瞄
    );

    控制器.设置提前(
        当前参数.提前.Y.最大累计,
        当前参数.提前.Y.转向最小速度阈值,
        当前参数.提前.Y.Y轴预瞄
    );
  
    // 获取热键配置
    std::string 锁定热键1 = 当前参数.热键更新.T.锁定热键1;
    std::string 锁定热键2 = 当前参数.热键更新.T.锁定热键2;
    std::string 锁定热键3 = 当前参数.热键更新.T.锁定热键3;
    std::string 锁定热键4 = 当前参数.热键更新.T.锁定热键4;
    std::string 锁定热键5 = 当前参数.热键更新.T.锁定热键5;

    // 转换热键为键码
    int 键码1 = 参数管理器::字符串到键码(锁定热键1);
    int 键码2 = 参数管理器::字符串到键码(锁定热键2);
    int 键码3 = 参数管理器::字符串到键码(锁定热键3);
    int 键码4 = 参数管理器::字符串到键码(锁定热键4);
    int 键码5 = 参数管理器::字符串到键码(锁定热键5);

    std::string 目标模型 = 当前参数.模型名.x.目标模型;
    std::string 路径模型 = 当前参数.模型名.x.路径模型;
    std::cout << "目标模型 " << 目标模型 << std::endl;
    std::cout << "路径模型 " << 路径模型 << std::endl;
    std::cout << "[参数] 配置已更新\n";

    // 构建模型文件路径
    std::string 目标模型路径_opp = "model\\" + 目标模型 + ".opp";
    std::string 目标模型路径_nbb = "model\\" + 目标模型 + ".nbb";
    std::string 轨迹模型路径_opp = "model\\" + 路径模型 + ".opp";
    std::string 轨迹模型路径_nbb = "model\\" + 路径模型 + ".nbb";
    std::cout << "轨迹模型opp路径: " << 轨迹模型路径_opp << std::endl;
    std::cout << "轨迹模型nbb路径: " << 轨迹模型路径_nbb << std::endl;

    // 初始化模型 - 使用 c_str() 将 std::string 转换为 const char*
  //  std::cout << "初始化轨迹模型: " << 轨迹模型路径_opp << ", " << 轨迹模型路径_nbb << std::endl;
    luj.init(轨迹模型路径_opp.c_str(), 轨迹模型路径_nbb.c_str(), 轨迹类型);

  //  std::cout << "初始化目标检测模型: " << 目标模型路径_opp << ", " << 目标模型路径_nbb << std::endl;
    yolo.init(目标模型路径_opp.c_str(), 目标模型路径_nbb.c_str(), 线程数, yolo版本选择);
    cap.init(截图大小, 截图大小, 截图方式);
    Beep(400, 400);  // 低音提示
    std::string _ip = 当前参数.系统.I._ip;
    std::string _port = 当前参数.系统.I._port;
    std::string _uuid = 当前参数.系统.I._uuid;
    std::string name = 当前参数.系统.I.name;
    std::cout << "初始化移动端: " << _ip << ", " << _port << ", " << _uuid << std::endl;
    mmove.init(_ip, _port, _uuid, name, 移动方式);
    std::vector<double> dist_list(10000);  // 初始化100个0.0
    std::vector<Object> results;
    results.reserve(10000);                // 预分配100个Object的内存（不初始化）
    std::vector<Object> results_copy;
    results_copy.reserve(10000);           // 同上
    std::vector<Aim> aims;
    aims.reserve(10000);                   // 预分配Aim
    std::vector<Aim> 有效目标;
    有效目标.reserve(10000);               // 预分配
    配置.切换监视状态();
    static auto 上次END按下时间 = std::chrono::steady_clock::now();
    static bool END键已按下 = false;
    Beep(600, 300);  // 低音提示
    // 显示当前状态
    std::cout << "配置文件监视: "
        << (配置.正在监视() ? "已启用" : "已禁用")
        << std::endl;

    while (true)
    
    {
        bool 当前END按下 = (GetAsyncKeyState(VK_END) & 0x8000) != 0;
        if (当前END按下) {
            auto 当前时间 = std::chrono::steady_clock::now();
            auto 时间间隔 = std::chrono::duration_cast<std::chrono::milliseconds>(当前时间 - 上次END按下时间).count();

            // 只有当按键是新的按下（之前未按下）并且超过防抖间隔时才触发
            if (!END键已按下 && 时间间隔 > 200) { // 200ms防抖
                END键已按下 = true;
                上次END按下时间 = 当前时间;

                // 切换监视状态
                配置.切换监视状态();

                // 播放声音反馈
                Beep(600, 300);

                // 显示当前状态
                std::cout << "配置文件监视: "
                    << (配置.正在监视() ? "已启用" : "已禁用")
                    << std::endl;
            }
        }
        else {
            // 按键释放时重置状态
            END键已按下 = false;
        }
        参数.状态轮询();

        if (参数.是否需要更新()) {
            参数.更新全部参数();

            // 读取最新参数（一次性结构体打包好）
            const auto& 当前参数 = 参数.获取当前参数();
            // 解包赋值（只在ini变动后执行一次）
            截图大小 = 当前参数.系统.I.截图大小;
            线程数 = 当前参数.系统.I.线程数;
            yolo版本选择 = 当前参数.系统.I.yolo版本选择;
            轨迹类型 = 当前参数.系统.I.轨迹类型;
            截图方式 = 当前参数.系统.I.截图方式;
            移动方式 = 当前参数.系统.I.移动方式;
            dt = 当前参数.系统.I.dt;
            锁定范围 = 当前参数.系统.I.锁定范围;
            上侧瞄准比例 = 当前参数.系统.I.上侧瞄准比例;
            下侧瞄准比例 = 当前参数.系统.I.下侧瞄准比例;
          //  是否压枪 = 当前参数.系统.I.是否压枪;
            置信度 = 当前参数.系统.I.置信度;
            预览窗口 = 当前参数.系统.I.预览窗口;
            //启动扳机 = 当前参数.系统.I.启动扳机;
            最低 = 当前参数.系统.I.最低;
            最高 = 当前参数.系统.I.最高;
           // mmove.设置随机射速范围(当前参数.系统.I.最低, 当前参数.系统.I.最高);

            // 更新 PID 控制器参数
            pid_x.更新参数(当前参数.控制器.X.比例,
                当前参数.控制器.X.积分,
                当前参数.控制器.X.微分);

            pid_y.更新参数(当前参数.控制器.Y.比例,
                当前参数.控制器.Y.积分,
                当前参数.控制器.Y.微分);

            // 更新 PID 平滑因子
            pid_x.设置平滑因子(当前参数.精调.X.输出平滑);
            pid_y.设置平滑因子(当前参数.精调.Y.输出平滑);

            // 更新 PID 底层参数
            pid_x.底层参数(
                当前参数.底参.X.达标误差阈值,
                当前参数.底参.X.速度倍率,
                当前参数.底参.X.最小系数,
                当前参数.底参.X.最大系数,
                当前参数.底参.X.过渡锐度,
                当前参数.底参.X.动态过渡中点,
                当前参数.底参.X.最小数据量,
                当前参数.底参.X.误差变化容限
            );

            pid_y.底层参数(
                当前参数.底参.Y.达标误差阈值,
                当前参数.底参.Y.速度倍率,
                当前参数.底参.Y.最小系数,
                当前参数.底参.Y.最大系数,
                当前参数.底参.Y.过渡锐度,
                当前参数.底参.Y.动态过渡中点,
                当前参数.底参.Y.最小数据量,
                当前参数.底参.Y.误差变化容限
            );

            // 提前量设置
            控制器.设置提前(
                当前参数.提前.X.最大累计,
                当前参数.提前.X.转向最小速度阈值,
                当前参数.提前.X.Y轴预瞄
            );

            控制器.设置提前(
                当前参数.提前.Y.最大累计,
                当前参数.提前.Y.转向最小速度阈值,
                当前参数.提前.Y.Y轴预瞄
            );
          
            std::cout << "[参数] 配置已更新\n" << std::endl;
        }   // 检查是否有热键按下
        bool 热键按下 = false;
        if (键码1 != -1 && (GetAsyncKeyState(键码1) & 0x8000)) 热键按下 = true;
        if (键码2 != -1 && (GetAsyncKeyState(键码2) & 0x8000)) 热键按下 = true;
        if (键码3 != -1 && (GetAsyncKeyState(键码3) & 0x8000)) 热键按下 = true;
        if (键码4 != -1 && (GetAsyncKeyState(键码4) & 0x8000)) 热键按下 = true;
        if (键码5 != -1 && (GetAsyncKeyState(键码5) & 0x8000)) 热键按下 = true;

        dist_list.clear();
        results.clear();  // 清空复用
        results_copy.clear();
        aims.clear();
        有效目标.clear();
        // 捕获图像

        cv::Mat img = cap.capture();
        if (img.empty()) {
            std::cout << "无截图" << std::endl;
        }

        yolo.detect(img, results, 截图大小, 置信度, 0.3);
        tracker.updateFrame(results);
        tracker.getTracks(results_copy);
        // 解析检测结果，绘制检测框及标签，并计算目标中心
        for (const auto& obj : results_copy) {
            Aim aim;
            aim.label = obj.label;
            aim.centerX = obj.rect.x + obj.rect.width / 2;
            aim.centerY = obj.rect.y + obj.rect.height / 2;
            aim.origin_x = obj.rect.x;
            aim.origin_y = obj.rect.y;
            aim.height = obj.rect.height;
            aim.width = obj.rect.width;
            aim.track_id = obj.track_id;
            aims.push_back(aim);
            // 绘制检测框及标签
            cv::rectangle(img, obj.rect, cv::Scalar(0, 255, 0), 2);

            // 只显示label
            std::string labelText = std::to_string(obj.label);

            // 获取文本尺寸
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(labelText, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);

            // 强制X轴间隔：按label值乘以10像素
            int text_x = obj.rect.x + (obj.label * 10);

            // 固定Y轴位置（框上方10像素）
            int text_y = obj.rect.y - 10;
            if (text_y < 0) text_y = obj.rect.y + 20; // 如果超出图像顶部则显示在框内

            // 绘制文本（无背景框）
            cv::putText(img, labelText,
                cv::Point(text_x, text_y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);

        }
        目标筛选器.处理快捷键();

        // 输出当前筛选类别名字，调试用
        有效目标 = 目标筛选器.筛选(aims);
        // 如果有符合目标，则选择距离图像中心最近的目标进行处理
        static bool 上键已按下 = false;
        double 当前瞄准比例 = 使用上侧瞄准比例 ? 上侧瞄准比例 : 下侧瞄准比例;
        if (GetAsyncKeyState(VK_UP) & 0x8000) {
            if (!上键已按下) {
                使用上侧瞄准比例 = true;
                Beep(600, 100); // 高音提示使用上侧比例
                上键已按下 = true;
            }
        }
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
            if (!上键已按下) {
                使用上侧瞄准比例 = false;
                Beep(300, 400); // 低音提示使用下侧比例
                上键已按下 = true;
            }
        }
        else {
            上键已按下 = false;
        }
        if (!有效目标.empty()) {
            // 调用目标选择函数，获取选中目标和误差
  
            auto 选择结果 = 选择目标(有效目标, offset_x, offset_y);
            Aim nearest_aim = 选择结果.选中目标;
            double error_x = 选择结果.横向误差;
            double error_y = 选择结果.纵向误差;

            bool firing = mobile.isFiring() != 0;
            bool targetExists = !有效目标.empty();

            if (targetExists) {
            /*    // 目标存在时的参数获取
                if (是否压枪 == 1) {
                    error_y = y压枪.update(true, firing, nearest_aim.centerY, offset_y,
                        nearest_aim.height, 当前瞄准比例);
                }
                else {}*/
                    error_y = (nearest_aim.centerY - offset_y + nearest_aim.height / 2) -
                        (nearest_aim.height * 当前瞄准比例);
                
            }
            else {
                // 无目标时重置状态
                //currentErrorY = y压枪.update(false, false, 0, 0, 0, 0);
                pid_x.重置();
                pid_y.重置();
            }
            if (!firing && lastFireState) {
             //   currentErrorY = y压枪.update(false, false, 0, 0, 0, 0); // 压枪重置

                pid_x.重置();
                pid_y.重置();
            }
            lastFireState = firing; // 更新状态

            double 最近目标宽度 = 选择结果.目标宽度;
            double 最近目标高度 = 选择结果.目标高度;
           /* if (启动扳机 == 1) {
                mmove.扳机更新(
                    error_x,          // X轴误差（目标中心 - 准星中心）
                    error_y,          // Y轴误差
                    最近目标宽度, // 目标框宽度
                    最近目标高度,// 目标框高度
                    targetExists      // 是否有目标
                );
            }
            if (启动扳机 == 2) {
                if (热键按下) {
                    mmove.扳机更新(
                        error_x,          // X轴误差（目标中心 - 准星中心）
                        error_y,          // Y轴误差
                        最近目标宽度, // 目标框宽度
                        最近目标高度,// 目标框高度
                        targetExists      // 是否有目标
                    );
                }
            }*/
            double move_x = pid_x.控制循环(error_x, dt, 最近目标宽度, 截图大小);
            double move_y = pid_y.控制循环(error_y, dt, 最近目标宽度, 截图大小);

     
            //auto end = std::chrono::high_resolution_clock::now();
            //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            //std::cout << "PID 计算耗时: " << duration.count() << " 微秒" << std::endl;
            //std::cout << "move_x " << move_x << std::endl;

            double 速度x = -pid_x.获取速度();
            double 速度y = -pid_y.获取速度() * 0.5; // y轴比例
            //std::cout << "速度速度x " << 速度x << std::endl;
            // 当目标在设定范围内时，检测右键或 Shift 键触发动作
            //std::cout << "偏移 - X: " << error_x << ",偏移 - Y: " << currentErrorY << std::endl;
            double range_squared = 锁定范围 * 锁定范围; // 瞄准范围
            if (targetExists && error_x * error_x + error_y * error_y < range_squared) {
                if (热键按下) {
                    std::vector<std::pair<double, double>> path = luj.predict(move_x, move_y);
                   
                    for (const auto& p : path) {
                        mmove.move(p.first, p.second);

                    }
                    控制器.更新偏移量(offset_x, offset_y, offset, dt, 速度x, 速度y, 状态);
                }
                else
                {
                   
                    pid_x.重置();
                    pid_y.重置();

                }
            }
            else
            {
          
                pid_x.重置();
                pid_y.重置();

            }
        }
        else
        {
            pid_x.重置();
            pid_y.重置();

        }
        // 在循环外部声明窗口状态
        static bool 窗口已创建 = false;

        // 在循环内部修改预览逻辑
        if (预览窗口) {
            if (!窗口已创建) {
                cv::namedWindow(窗口名, cv::WINDOW_AUTOSIZE);
                窗口已创建 = true;
            }
            cv::imshow(窗口名, img);
            cv::waitKey(1);
        }
        else {
            if (窗口已创建) {
                cv::destroyWindow(窗口名);
                窗口已创建 = false;
            }
        }

    }
    Logger::Close();
    return 0;
}
