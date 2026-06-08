#ifndef MOBILE_H
#define MOBILE_H
#include "cat_n\cat_net.h"
#include "cat_o\cat_o.h"
#include "cat_t\cat_t.h"
#include "km_b\km_b.h"
#include "km_n\km_n.h"
#include "km_p\km_p.h"
#include "Ghub\Ghub.h"
#include "Send\Send.h"
#include "CH9329.h"
#include "makcu.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <unordered_map>
#include <string>

class Mobile
{
public:
    Mobile();
    ~Mobile();
    void init(std::string _ip, std::string _port, std::string _uuid, std::string& name, int modes);
    void move(double x, double y);
    int monitor(std::string vkey, bool state = false);
    void mouseButton(std::string code, uint16_t value);
    void keyboardButton(std::string code, uint16_t value);
    bool isFiring() const {
        return physicalFiring || simulatedFiring;
    }
    const int MIN_ALLOWED_INTERVAL = 180;
    const int MAX_ALLOWED_INTERVAL = 280;
    void 设置随机射速范围(int 最小ms, int 最大ms) {
        最小射速 = std::max(最小ms, MIN_ALLOWED_INTERVAL);
        最大射速 = std::min(最大ms, MAX_ALLOWED_INTERVAL);
        if (最小射速 > 最大射速) {
            最大射速 = 最小射速;
        }
        intervalMs = 随机间隔();
        lastTriggerTime = std::chrono::steady_clock::now();
    }
    void 扳机更新(double 误差x, double 误差y,
        double 框宽, double 框高,
        bool targetExists)
    {
        if (!targetExists) return;
        auto now = std::chrono::steady_clock::now();
        auto 距离 = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTriggerTime).count();
        double 容差x = 框宽 / 2.0;
        double 容差y = 框高 / 2.0;
        bool 在框内 = (std::abs(误差x) <= 容差x) && (std::abs(误差y) <= 容差y);
        if (在框内 && 距离 >= intervalMs) {
            lastTriggerTime = now;
            intervalMs = 随机间隔();
            模拟左键点击();
        }
    }
    int getMode() const { return mode; }
    bool isMakcuConnected() const {
        if (mode == 9) {
            return box::MouseMakcu::getInstance().isConnect();
        }
        return false;
    }

private:
    int mode = -1;
    CatNet cat_net;
    CatO cat_one;
    CatT cat_two;
    KMB kmb;
    KMPLink km_p;
    WinMouseMove WinSend;
    CH9329* ch9329 = nullptr;
    std::unordered_map<std::string, int> falseCounter;
    std::unordered_map<std::string, bool> keyState;
    const int FALSE_THRESHOLD = 10;
    int 最小射速 = 180;
    int 最大射速 = 280;
    int intervalMs = 200;
    std::chrono::steady_clock::time_point lastTriggerTime;
    std::mt19937 rng;
    std::atomic<bool> running;
    std::atomic<bool> physicalFiring;
    std::atomic<bool> simulatedFiring;
    std::thread detectionThread;
    int 随机间隔() {
        std::uniform_int_distribution<int> dist(最小射速, 最大射速);
        return dist(rng);
    }
    void detectPhysicalMouse();
    void 模拟左键点击();
    double ratio = 0;
};
#endif
