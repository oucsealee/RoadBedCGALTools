#include "logging.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

namespace CGAL_OSG_TOOL_NS {

// 静态成员初始化
LogLevel Logger::currentLevel = LogLevel::INFO;
std::unordered_map<std::string, std::vector<LogEntry>> Logger::logBuffer;
std::mutex Logger::logMutex;
bool Logger::enableConsoleOutput = true;

/**
 * 设置日志级别
 */
void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLevel = level;
}

/**
 * 获取当前日志级别
 */
LogLevel Logger::getLogLevel() {
    std::lock_guard<std::mutex> lock(logMutex);
    return currentLevel;
}

/**
 * 设置是否启用控制台输出
 */
void Logger::setConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex);
    enableConsoleOutput = enable;
}

/**
 * 获取当前时间字符串
 */
static std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * 获取线程ID字符串
 */
std::string Logger::getThreadIdString() {
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream ss;
    ss << tid;
    return ss.str();
}

/**
 * 获取日志级别字符串
 */
std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG: return "[DEBUG]";
    case LogLevel::INFO: return "[INFO]";
    case LogLevel::WARNING: return "[WARNING]";
    case LogLevel::ERROR: return "[ERROR]";
    case LogLevel::FATAL: return "[FATAL]";
    default: return "[UNKNOWN]";
    }
}

/**
 * 内部日志记录方法
 */
void Logger::log(LogLevel level, const std::string& message) {
    if (level < currentLevel) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string timestamp = getCurrentTime();
    std::string threadId = getThreadIdString();
    
    // 按线程ID分组存储日志
    logBuffer[threadId].emplace_back(level, timestamp, threadId, message);
    
    // 如果启用控制台输出
    if (enableConsoleOutput) {
        std::string levelStr = getLevelString(level);
        if (level >= LogLevel::WARNING) {
            std::cerr << timestamp << " [T:" << threadId << "] " << levelStr << " " << message << std::endl;
        } else {
            std::cout << timestamp << " [T:" << threadId << "] " << levelStr << " " << message << std::endl;
        }
    }
}

/**
 * 输出调试日志
 */
void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

/**
 * 输出信息日志
 */
void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

/**
 * 输出警告日志
 */
void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

/**
 * 输出错误日志
 */
void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

/**
 * 输出致命错误日志
 */
void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

/**
 * 获取所有日志条目
 */
std::vector<LogEntry> Logger::getLogs() {
    std::lock_guard<std::mutex> lock(logMutex);
    std::vector<LogEntry> result;
    for (const auto& pair : logBuffer) {
        result.insert(result.end(), pair.second.begin(), pair.second.end());
    }
    return result;
}

/**
 * 获取指定级别及以上的日志条目
 */
std::vector<LogEntry> Logger::getLogs(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::vector<LogEntry> result;
    for (const auto& pair : logBuffer) {
        for (const auto& entry : pair.second) {
            if (entry.level >= level) {
                result.push_back(entry);
            }
        }
    }
    return result;
}

/**
 * 获取指定线程的日志条目
 */
std::vector<LogEntry> Logger::getLogsByThread(const std::string& threadId) {
    std::lock_guard<std::mutex> lock(logMutex);
    auto it = logBuffer.find(threadId);
    if (it != logBuffer.end()) {
        return it->second;
    }
    return {};
}

/**
 * 获取当前线程的日志条目
 */
std::vector<LogEntry> Logger::getCurrentThreadLogs() {
    std::string threadId = getThreadIdString();
    return getLogsByThread(threadId);
}

/**
 * 获取日志条目数量
 */
size_t Logger::getLogCount() {
    std::lock_guard<std::mutex> lock(logMutex);
    size_t count = 0;
    for (const auto& pair : logBuffer) {
        count += pair.second.size();
    }
    return count;
}

/**
 * 获取指定线程的日志条目数量
 */
size_t Logger::getLogCountByThread(const std::string& threadId) {
    std::lock_guard<std::mutex> lock(logMutex);
    auto it = logBuffer.find(threadId);
    if (it != logBuffer.end()) {
        return it->second.size();
    }
    return 0;
}

/**
 * 清空日志缓存
 */
void Logger::clearLogs() {
    std::lock_guard<std::mutex> lock(logMutex);
    logBuffer.clear();
}

/**
 * 清空指定线程的日志缓存
 */
void Logger::clearLogsByThread(const std::string& threadId) {
    std::lock_guard<std::mutex> lock(logMutex);
    logBuffer.erase(threadId);
}

/**
 * 获取日志内容作为字符串
 */
std::string Logger::getLogString() {
    std::lock_guard<std::mutex> lock(logMutex);
    std::stringstream ss;
    for (const auto& pair : logBuffer) {
        for (const auto& entry : pair.second) {
            ss << entry.timestamp << " [T:" << entry.threadId << "] " 
               << getLevelString(entry.level) << " " << entry.message << "\n";
        }
    }
    return ss.str();
}

/**
 * 获取指定线程的日志内容作为字符串
 */
std::string Logger::getLogStringByThread(const std::string& threadId) {
    std::lock_guard<std::mutex> lock(logMutex);
    auto it = logBuffer.find(threadId);
    if (it == logBuffer.end()) {
        return "";
    }
    
    std::stringstream ss;
    for (const auto& entry : it->second) {
        ss << entry.timestamp << " [T:" << entry.threadId << "] " 
           << getLevelString(entry.level) << " " << entry.message << "\n";
    }
    return ss.str();
}

/**
 * 获取当前线程的日志内容作为字符串
 */
std::string Logger::getCurrentThreadLogString() {
    std::string threadId = getThreadIdString();
    return getLogStringByThread(threadId);
}

} // namespace CGAL_OSG_TOOL_NS
