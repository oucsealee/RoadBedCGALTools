#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>

namespace CGAL_OSG_TOOL_NS {

/**
 * 日志级别
 */
enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

/**
 * 日志条目结构
 */
struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string threadId;
    std::string message;

    LogEntry(LogLevel l, const std::string& t, const std::string& tid, const std::string& m)
        : level(l), timestamp(t), threadId(tid), message(m) {}
};

/**
 * 日志工具类
 * 支持线程安全的日志记录，日志先缓存，可在操作完成后获取
 * 支持区分不同线程的日志信息
 */
class Logger {
private:
    static LogLevel currentLevel;
    static std::unordered_map<std::string, std::vector<LogEntry>> logBuffer;
    static std::mutex logMutex;
    static bool enableConsoleOutput;

public:
    /**
     * 设置日志级别
     * @param level 日志级别
     */
    static void setLogLevel(LogLevel level);

    /**
     * 获取当前日志级别
     * @return 日志级别
     */
    static LogLevel getLogLevel();

    /**
     * 设置是否启用控制台输出
     * @param enable true启用，false禁用
     */
    static void setConsoleOutput(bool enable);

    /**
     * 输出调试日志
     * @param message 日志消息
     */
    static void debug(const std::string& message);

    /**
     * 输出信息日志
     * @param message 日志消息
     */
    static void info(const std::string& message);

    /**
     * 输出警告日志
     * @param message 日志消息
     */
    static void warning(const std::string& message);

    /**
     * 输出错误日志
     * @param message 日志消息
     */
    static void error(const std::string& message);

    /**
     * 输出致命错误日志
     * @param message 日志消息
     */
    static void fatal(const std::string& message);

    /**
     * 获取所有日志条目
     * @return 日志条目向量
     */
    static std::vector<LogEntry> getLogs();

    /**
     * 获取指定级别及以上的日志条目
     * @param level 最低日志级别
     * @return 日志条目向量
     */
    static std::vector<LogEntry> getLogs(LogLevel level);

    /**
     * 获取指定线程的日志条目
     * @param threadId 线程ID
     * @return 日志条目向量
     */
    static std::vector<LogEntry> getLogsByThread(const std::string& threadId);

    /**
     * 获取当前线程的日志条目
     * @return 日志条目向量
     */
    static std::vector<LogEntry> getCurrentThreadLogs();

    /**
     * 获取日志条目数量
     * @return 日志条目数量
     */
    static size_t getLogCount();

    /**
     * 获取指定线程的日志条目数量
     * @param threadId 线程ID
     * @return 日志条目数量
     */
    static size_t getLogCountByThread(const std::string& threadId);

    /**
     * 清空日志缓存
     */
    static void clearLogs();

    /**
     * 清空指定线程的日志缓存
     * @param threadId 线程ID
     */
    static void clearLogsByThread(const std::string& threadId);

    /**
     * 获取日志内容作为字符串
     * @return 格式化的日志字符串
     */
    static std::string getLogString();

    /**
     * 获取指定线程的日志内容作为字符串
     * @param threadId 线程ID
     * @return 格式化的日志字符串
     */
    static std::string getLogStringByThread(const std::string& threadId);

    /**
     * 获取当前线程的日志内容作为字符串
     * @return 格式化的日志字符串
     */
    static std::string getCurrentThreadLogString();

    /**
     * 获取当前线程ID字符串
     * @return 当前线程ID字符串
     */
    static std::string getThreadIdString();

private:
    /**
     * 内部日志记录方法
     */
    static void log(LogLevel level, const std::string& message);

    /**
     * 获取日志级别字符串
     */
    static std::string getLevelString(LogLevel level);
};

} // namespace CGAL_OSG_TOOL_NS

#endif // LOGGING_H
