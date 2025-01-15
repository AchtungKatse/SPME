#pragma once

namespace SPME {
    #define Assert(condition, ...) if (!(condition)) { ::spdlog::error(__VA_ARGS__); abort(); } 
}
