#pragma once
#include <stdlib.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_TRACE = 1,
    LOG_LEVEL_INFO  = 2,
    LOG_LEVEL_WARN  = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5,
} log_level;

void LoggingInitialize();
void LoggingShutdown();
void Log(log_level level, const char* format, ...);

#define LogDebug(...)   Log(LOG_LEVEL_DEBUG, __VA_ARGS__);
#define LogTrace(...)   Log(LOG_LEVEL_TRACE, __VA_ARGS__);
#define LogInfo(...)    Log(LOG_LEVEL_INFO, __VA_ARGS__);
#define LogWarn(...)    Log(LOG_LEVEL_WARN, __VA_ARGS__);
#define LogError(...)   Log(LOG_LEVEL_ERROR, __VA_ARGS__);
#define LogFatal(...)   Log(LOG_LEVEL_FATAL, __VA_ARGS__);
#define Assert(condition, ...) if (!(condition)) { LogError(__FILE__ ":%d", __LINE__); LogError(__VA_ARGS__); abort(); } 
