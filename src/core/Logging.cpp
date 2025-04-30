#include "core/Logging.h"
#include <cstdarg>
#include <cstring>

namespace SPMEditor {
    
    struct LoggingContext {
        static constexpr u64 MessageBufferSize = 0x1000000;

        char messageBuffer[MessageBufferSize];
    };

    static LoggingContext* context;

    void LoggingInitialize() {
        context = new LoggingContext();
        memset(context, 0, sizeof(LoggingContext));
    }

    void Log(LogLevel level, const char* format, ...) {
        Assert(context, "Cannot write logs. Logging system never initialized");
        const char* level_strings[6] = {
            "\x1B[32m[DEBUG]: ",
            "\x1B[36m[TRACE]: ",
            "\x1B[37m[INFO]:  ",
            "\x1B[33m[WARN]:  ",
            "\x1B[91m[ERROR]: ",
            "\x1B[31m[FATAL]: ",
        };

        // Format original message.
        // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
        // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
        // which is the type GCC/Clang's va_start expects.
        __builtin_va_list arg_ptr;
        va_start(arg_ptr, format);
        vsnprintf(context->messageBuffer, LoggingContext::MessageBufferSize, format, arg_ptr);
        va_end(arg_ptr);

        printf("%s%s\n", level_strings[(int)level], context->messageBuffer);
    }

    void LoggingShutdown() {
        delete context;
    }
}
