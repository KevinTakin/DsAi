#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <windows.h>
#include <opencv2/opencv.hpp>

class RecoilController {
private:
    // 压枪核心参数
    double recoil_offset = 0.0;
    bool isRecoilActive = false;
    bool isPaused = false;
    std::chrono::steady_clock::time_point stateStartTime;

    // 时间控制参数
    int delayBeforeRecoil = 0;
    int recoilDuration = 0;
    int pauseDuration = 0;

    // 单点射击控制
    std::chrono::steady_clock::time_point shotStartTime;
    bool isSingleShot = false;
    int singleShotThreshold = 150; // 单点判定阈值(ms)
    int singleShotDelay = 80;      // 单点专用延迟(ms)
    bool lastFiringState = false;

    // 控制参数（去掉 const，改为可变成员）
    double recoil_factor = 0.3;
    double recovery_factor = 0.7;

    // 随机数系统
    std::mt19937 gen;
    std::uniform_int_distribution<> delayDist;
    std::uniform_int_distribution<> durationDist;
    std::uniform_int_distribution<> pauseDist;

public:
    // 构造函数：用默认参数初始化
    RecoilController(
        double recoilFactor = 0.3,
        double recoveryFactor = 0.7,
        int minDelay = 50,
        int maxDelay = 150,
        int minDuration = 100,
        int maxDuration = 300,
        int minPause = 200,
        int maxPause = 500,
        int singleShotThreshold_ = 150,
        int singleShotDelay_ = 80)
        : recoil_factor(recoilFactor),
        recovery_factor(recoveryFactor),
        gen(std::random_device{}()),
        delayDist(minDelay, maxDelay),
        durationDist(minDuration, maxDuration),
        pauseDist(minPause, maxPause),
        singleShotThreshold(singleShotThreshold_),
        singleShotDelay(singleShotDelay_)
    {
        stateStartTime = std::chrono::steady_clock::now();
    }

    // 新增：动态更新参数接口
    void 更新参数(
        double recoilFactor,
        double recoveryFactor,
        int minDelay,
        int maxDelay,
        int minDuration,
        int maxDuration,
        int minPause,
        int maxPause,
        int singleShotThreshold_,
        int singleShotDelay_)
    {
        recoil_factor = recoilFactor;
        recovery_factor = recoveryFactor;
        delayDist = std::uniform_int_distribution<>(minDelay, maxDelay);
        durationDist = std::uniform_int_distribution<>(minDuration, maxDuration);
        pauseDist = std::uniform_int_distribution<>(minPause, maxPause);
        singleShotThreshold = singleShotThreshold_;
        singleShotDelay = singleShotDelay_;
    }
    double update(bool targetExists,
        bool firing,
        double nearestAimCenterY,
        double offset,
        double nearestAimHeight,
        double UpperRatio = 0.3)
    {
        auto now = std::chrono::steady_clock::now();

        // 单点射击检测逻辑
        detectSingleShot(firing, now);

        if (!targetExists) {
            resetState();
            return calculateOriginalError(nearestAimCenterY, offset, nearestAimHeight, UpperRatio);
        }

        // 单点模式延迟处理
        if (handleSingleShotDelay(now)) {
            return calculateOriginalError(nearestAimCenterY, offset, nearestAimHeight, UpperRatio);
        }

        // 状态机处理
        if (isPaused) {
            handlePauseState(now);
        }
        else if (firing) {
            handleFiringState(now);
        }
        else {
            handleNonFiringState();
        }

        return calculateCompensatedError(nearestAimCenterY, offset, nearestAimHeight, UpperRatio);
    }

private:
    // 单点射击检测
    void detectSingleShot(bool firing, std::chrono::steady_clock::time_point now) {
        // 检测按下瞬间
        if (firing && !lastFiringState) {
            shotStartTime = now;
        }
        lastFiringState = firing;

        // 计算按压持续时间
        auto pressDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - shotStartTime).count();

        isSingleShot = !firing && (pressDuration < singleShotThreshold);
    }

    // 处理单点延迟
    bool handleSingleShotDelay(std::chrono::steady_clock::time_point now) {
        if (!isSingleShot) return false;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - shotStartTime).count();

        // 在延迟期间阻止压枪
        if (elapsed < singleShotDelay) {
            // 调试输出
            /*std::cout << "[SingleShot] Delay remaining: "
                      << singleShotDelay - elapsed
                      << "ms" << std::endl;*/
            return true;
        }
        return false;
    }

    // 状态重置
    void resetState() {
        recoil_offset = 0.0;
        isRecoilActive = false;
        isPaused = false;
        delayBeforeRecoil = 0;
        recoilDuration = 0;
        pauseDuration = 0;
        isSingleShot = false;
    }

    // 暂停状态处理
    void handlePauseState(std::chrono::steady_clock::time_point now) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - stateStartTime).count();
        if (elapsed >= pauseDuration) {
            isPaused = false;
            delayBeforeRecoil = delayDist(gen);
            stateStartTime = now;
        }
        else {
            recoil_offset *= recovery_factor;
        }
    }

    // 射击状态处理
    void handleFiringState(std::chrono::steady_clock::time_point now) {
        if (!isRecoilActive) {
            if (delayBeforeRecoil == 0) {
                delayBeforeRecoil = delayDist(gen);
                stateStartTime = now;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - stateStartTime).count();
            if (elapsed >= delayBeforeRecoil) {
                activateRecoil(now);
            }
        }
        else {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - stateStartTime).count();
            if (elapsed >= recoilDuration) {
                activatePause(now);
            }
            else {
                recoil_offset += recoil_factor;
            }
        }
    }

    // 非射击状态处理
    void handleNonFiringState() {
        recoil_offset *= recovery_factor;
        isRecoilActive = false;
        delayBeforeRecoil = 0;
        recoilDuration = 0;
    }

    // 激活压枪
    void activateRecoil(std::chrono::steady_clock::time_point now) {
        isRecoilActive = true;
        recoilDuration = durationDist(gen);
        stateStartTime = now;
    }

    // 激活暂停
    void activatePause(std::chrono::steady_clock::time_point now) {
        isRecoilActive = false;
        isPaused = true;
        pauseDuration = pauseDist(gen);
        stateStartTime = now;
    }

    // 计算原始误差（无压枪补偿）
    double calculateOriginalError(double centerY, double offset, double height, double upperRatio) const {
        return centerY - offset + height / 2 - height * upperRatio;
    }

    // 计算补偿后误差
    double calculateCompensatedError(double centerY, double offset, double height, double upperRatio) const {
        return calculateOriginalError(centerY, offset, height, upperRatio) + recoil_offset;
    }
};
