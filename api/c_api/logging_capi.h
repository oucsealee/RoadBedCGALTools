#ifndef LOGGING_CAPI_H
#define LOGGING_CAPI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 获取当前线程的日志信息
 * @return 日志信息字符串（需要调用 rb_free_log_string 释放内存），失败返回 NULL
 */
__declspec(dllexport) const char* rb_get_current_thread_log();

/**
 * 获取指定线程的日志信息
 * @param threadId 线程ID字符串
 * @return 日志信息字符串（需要调用 rb_free_log_string 释放内存），失败返回 NULL
 */
__declspec(dllexport) const char* rb_get_thread_log(const char* threadId);

/**
 * 获取所有日志信息
 * @return 日志信息字符串（需要调用 rb_free_log_string 释放内存），失败返回 NULL
 */
__declspec(dllexport) const char* rb_get_all_logs();

/**
 * 释放日志字符串内存
 * @param logString 由 rb_get_current_thread_log、rb_get_thread_log 或 rb_get_all_logs 返回的字符串
 */
__declspec(dllexport) void rb_free_log_string(const char* logString);

/**
 * 清空当前线程的日志
 */
__declspec(dllexport) void rb_clear_current_thread_log();

/**
 * 清空所有日志
 */
__declspec(dllexport) void rb_clear_all_logs();

#ifdef __cplusplus
}
#endif

#endif // LOGGING_CAPI_H
