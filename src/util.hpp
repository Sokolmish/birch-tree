#ifndef UTIL_HPP_INCLUDED__
#define UTIL_HPP_INCLUDED__

#include <fmt/format.h>

template <>
struct fmt::formatter<std::filesystem::path>: formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx) {
        return formatter<std::string_view>::format(path.string(), ctx);
    }
};

#endif /* UTIL_HPP_INCLUDED__ */
