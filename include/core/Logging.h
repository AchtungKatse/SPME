#pragma once

namespace SPMEditor {
    enum LogLevel {
        LOG_LEVEL_DEBUG = 0,
        LOG_LEVEL_TRACE = 1,
        LOG_LEVEL_INFO  = 2,
        LOG_LEVEL_WARN  = 3,
        LOG_LEVEL_ERROR = 4,
        LOG_LEVEL_FATAL = 5,
    };

    void LoggingInitialize();
    void LoggingShutdown();
    void Log(LogLevel level, const char* format, ...);

#define LogDebug(...)   Log(SPMEditor::LOG_LEVEL_DEBUG, __VA_ARGS__);
#define LogTrace(...)   Log(SPMEditor::LOG_LEVEL_TRACE, __VA_ARGS__);
#define LogInfo(...)    Log(SPMEditor::LOG_LEVEL_INFO, __VA_ARGS__);
#define LogWarn(...)    Log(SPMEditor::LOG_LEVEL_WARN, __VA_ARGS__);
#define LogError(...)   Log(SPMEditor::LOG_LEVEL_ERROR, __VA_ARGS__);
#define LogFatal(...)   Log(SPMEditor::LOG_LEVEL_FATAL, __VA_ARGS__);
#define Assert(condition, ...) if (!(condition)) { LogError(__FILE__ ":%d", __LINE__); LogError(__VA_ARGS__); abort(); } 
}
