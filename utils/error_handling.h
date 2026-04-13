#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <string>
#include <stdexcept>

namespace CGAL_OSG_TOOL_NS {

/**
 * 错误代码枚举
 */
enum class ErrorCode {
    SUCCESS = 0,             // 成功
    INVALID_INPUT = 1,       // 无效输入
    MEMORY_ERROR = 2,        // 内存错误
    GEOMETRY_ERROR = 3,      // 几何错误
    CGAL_ERROR = 4,          // CGAL库错误
    OSG_ERROR = 5,           // OSG库错误
    UNKNOWN_ERROR = 99       // 未知错误
};

/**
 * 错误处理类
 */
class ErrorHandler {
private:
    static ErrorCode lastError;
    static std::string lastErrorMessage;

public:
    /**
     * 设置错误
     * @param code 错误代码
     * @param message 错误信息
     */
    static void setError(ErrorCode code, const std::string& message);

    /**
     * 获取最后一次错误代码
     * @return 错误代码
     */
    static ErrorCode getLastError();

    /**
     * 获取最后一次错误信息
     * @return 错误信息
     */
    static std::string getErrorMessage();

    /**
     * 清除错误
     */
    static void clearError();

    /**
     * 检查是否有错误
     * @return 是否有错误
     */
    static bool hasError();
};

/**
 * 自定义异常类
 */
class CGALOSGException : public std::runtime_error {
private:
    ErrorCode errorCode;

public:
    /**
     * 构造函数
     * @param code 错误代码
     * @param message 错误信息
     */
    CGALOSGException(ErrorCode code, const std::string& message);

    /**
     * 获取错误代码
     * @return 错误代码
     */
    ErrorCode getErrorCode() const;
};

/**
 * 执行信息类
 */
class ExecutionInfo {
private:
    int successCount;
    int failureCount;
    std::string functionName;

public:
    /**
     * 构造函数
     * @param funcName 函数名称
     */
    ExecutionInfo(const std::string& funcName = "");

    /**
     * 增加成功计数
     */
    void incrementSuccess();

    /**
     * 增加失败计数
     */
    void incrementFailure();

    /**
     * 获取成功计数
     * @return 成功计数
     */
    int getSuccessCount() const;

    /**
     * 获取失败计数
     * @return 失败计数
     */
    int getFailureCount() const;

    /**
     * 获取函数名称
     * @return 函数名称
     */
    std::string getFunctionName() const;

    /**
     * 打印执行信息
     */
    void printInfo() const;
};

} // namespace CGAL_OSG_TOOL_NS

#endif // ERROR_HANDLING_H