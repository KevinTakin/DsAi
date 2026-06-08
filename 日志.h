#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <windows.h>
#include <mutex>

class Logger {
public:
    static bool Initialize(const std::string& logPath) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (isInitialized) return true; // 防止重复初始化

        logFilePath = logPath;
        isEnabled = true; // 启用日志系统

        // 清空旧日志
        std::ofstream clearFile(logFilePath, std::ios::out | std::ios::trunc);
        clearFile.close();

        // 重定向输出流
        oldCoutBuffer = std::cout.rdbuf();
        oldCerrBuffer = std::cerr.rdbuf();
        std::cout.rdbuf(logStream.rdbuf());
        std::cerr.rdbuf(logStream.rdbuf());

        // 写入初始日志头
        WriteToFile("========== 程序启动日志 " + GetCurrentTimeString() + " ==========");
        isInitialized = true;
        return true;
    }

    static void Log(const std::string& message) {
        if (!isEnabled) return; // 如果已关闭则忽略
        std::lock_guard<std::mutex> lock(logMutex);
        WriteToFile("[" + GetCurrentTimeString() + "] " + message);
    }

    static void Close() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (!isInitialized) return;

        // 恢复原始cout和cerr
        std::cout.rdbuf(oldCoutBuffer);
        std::cerr.rdbuf(oldCerrBuffer);

        // 写入结束日志
        WriteToFile("========== 日志系统关闭 " + GetCurrentTimeString() + " ==========");

        // 重置状态
        isEnabled = false;
        isInitialized = false;
    }

    static bool IsEnabled() {
        return isEnabled;
    }

private:
    static void WriteToFile(const std::string& content) {
        if (!isEnabled) return;

        std::ofstream file(logFilePath, std::ios::out | std::ios::app);
        if (file.is_open()) {
            file << content << "\n";
            file.close();
        }
    }

    static std::string GetCurrentTimeString() {
        std::time_t now = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    static std::string logFilePath;
    static std::mutex logMutex;
    static std::streambuf* oldCoutBuffer;
    static std::streambuf* oldCerrBuffer;
    static bool isEnabled;
    static bool isInitialized;

    // 自定义streambuf实现
    class LogStreamBuf : public std::streambuf {
    public:
        virtual int_type overflow(int_type c) override {
            if (c != EOF && Logger::IsEnabled()) { // 检查日志是否启用
                char ch = static_cast<char>(c);
                buffer += ch;
                if (ch == '\n') {
                    Logger::Log(buffer);
                    buffer.clear();
                }
            }
            return c;
        }

        virtual int sync() override {
            if (!buffer.empty() && Logger::IsEnabled()) {
                Logger::Log(buffer);
                buffer.clear();
            }
            return 0;
        }

    private:
        std::string buffer;
    };

    static LogStreamBuf logStreamBuf;
    static std::ostream logStream;
};

// 静态成员初始化
std::string Logger::logFilePath;
std::mutex Logger::logMutex;
std::streambuf* Logger::oldCoutBuffer = nullptr;
std::streambuf* Logger::oldCerrBuffer = nullptr;
Logger::LogStreamBuf Logger::logStreamBuf;
std::ostream Logger::logStream(&Logger::logStreamBuf);
bool Logger::isEnabled = false;
bool Logger::isInitialized = false;

#endif // LOG_UTIL_H