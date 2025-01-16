#pragma once

namespace SPME {
    #define LogTrace(...) ::spdlog::trace(__VA_ARGS__)
    #define LogInfo(...) ::spdlog::info(__VA_ARGS__)
    #define LogError(...) ::spdlog::error(__VA_ARGS__)
}
