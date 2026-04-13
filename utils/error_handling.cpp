#include "error_handling.h"
#include "logging.h"
#include <string>
#include <stdexcept>

namespace CGAL_OSG_TOOL_NS {

// 静态成员初始化
ErrorCode ErrorHandler::lastError = ErrorCode::SUCCESS;
std::string ErrorHandler::lastErrorMessage = "";

/**
 * 设置错误
 */
void ErrorHandler::setError(ErrorCode code, const std::string& message) {
    lastError = code;
    lastErrorMessage = message;
    Logger::error("Error: " + message);
}

/**
 * 获取最后一次错误代码
 */
ErrorCode ErrorHandler::getLastError() {
    return lastError;
}

/**
 * 获取最后一次错误信息
 */
std::string ErrorHandler::getErrorMessage() {
    return lastErrorMessage;
}

/**
 * 清除错误
 */
void ErrorHandler::clearError() {
    lastError = ErrorCode::SUCCESS;
    lastErrorMessage = "";
}

/**
 * 检查是否有错误
 */
bool ErrorHandler::hasError() {
    return lastError != ErrorCode::SUCCESS;
}

/**
 * 自定义异常类构造函数
 */
CGALOSGException::CGALOSGException(ErrorCode code, const std::string& message) 
    : std::runtime_error(message), errorCode(code) {
    ErrorHandler::setError(code, message);
}

/**
 * 获取错误代码
 */
ErrorCode CGALOSGException::getErrorCode() const {
    return errorCode;
}

/**
 * 执行信息构造函数
 */
ExecutionInfo::ExecutionInfo(const std::string& funcName) 
    : successCount(0), failureCount(0), functionName(funcName) {
}

/**
 * 增加成功计数
 */
void ExecutionInfo::incrementSuccess() {
    successCount++;
}

/**
 * 增加失败计数
 */
void ExecutionInfo::incrementFailure() {
    failureCount++;
}

/**
 * 获取成功计数
 */
int ExecutionInfo::getSuccessCount() const {
    return successCount;
}

/**
 * 获取失败计数
 */
int ExecutionInfo::getFailureCount() const {
    return failureCount;
}

/**
 * 获取函数名称
 */
std::string ExecutionInfo::getFunctionName() const {
    return functionName;
}

/**
 * 打印执行信息
 */
void ExecutionInfo::printInfo() const {
    if (!functionName.empty()) {
        Logger::info(functionName + " execution info:");
    }
    Logger::info("  Success count: " + std::to_string(successCount));
    Logger::info("  Failure count: " + std::to_string(failureCount));
}

} // namespace CGAL_OSG_TOOL_NS