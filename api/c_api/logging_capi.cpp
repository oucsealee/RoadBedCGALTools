#include "logging_capi.h"
#include "utils/logging.h"
#include <cstring>

extern "C" {

const char* rb_get_current_thread_log() {
    std::string logStr = CGAL_OSG_TOOL_NS::Logger::getCurrentThreadLogString();
    if (logStr.empty()) {
        return nullptr;
    }
    
    char* result = new char[logStr.size() + 1];
    std::strcpy(result, logStr.c_str());
    return result;
}

const char* rb_get_thread_log(const char* threadId) {
    if (!threadId) {
        return nullptr;
    }
    
    std::string logStr = CGAL_OSG_TOOL_NS::Logger::getLogStringByThread(threadId);
    if (logStr.empty()) {
        return nullptr;
    }
    
    char* result = new char[logStr.size() + 1];
    std::strcpy(result, logStr.c_str());
    return result;
}

const char* rb_get_all_logs() {
    std::string logStr = CGAL_OSG_TOOL_NS::Logger::getLogString();
    if (logStr.empty()) {
        return nullptr;
    }
    
    char* result = new char[logStr.size() + 1];
    std::strcpy(result, logStr.c_str());
    return result;
}

void rb_free_log_string(const char* logString) {
    if (logString) {
        delete[] logString;
    }
}

void rb_clear_current_thread_log() {
    CGAL_OSG_TOOL_NS::Logger::clearLogsByThread(
        CGAL_OSG_TOOL_NS::Logger::getThreadIdString());
}

void rb_clear_all_logs() {
    CGAL_OSG_TOOL_NS::Logger::clearLogs();
}

} // extern "C"
