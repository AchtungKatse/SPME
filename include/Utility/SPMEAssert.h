#pragma once

namespace SPMEditor {
    #define Assert(condition, ...) if (!(condition)) { ::spdlog::error("{}:{}", __FILE__, __LINE__); ::spdlog::error(__VA_ARGS__); abort(); } 
}
