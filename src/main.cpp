#include <vector>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/color.h>
#include "util.hpp"

namespace fs = std::filesystem;

static void initLsColors() {
    using tc = fmt::terminal_color;
    using emp = fmt::emphasis;

    std::unordered_map<std::string, fmt::text_style> ls_col{
            { "fi", fmt::text_style() },               // Regular file
            { "di", fmt::fg(tc::blue) | emp::bold },   // Directory
            { "ln", fmt::fg(tc::cyan) | emp::bold },   // Symlink
            { "pi", fmt::fg(tc::yellow) },             // FIFO
            { "bd", fmt::fg(tc::yellow) | emp::bold }, // Block device
            { "bd", fmt::fg(tc::yellow) | emp::bold }, // Character device
            { "or", fmt::fg(tc::red) | emp::bold },    // Orphan link
            { "mi", fmt::text_style() },               // Missing
            { "so", fmt::fg(tc::magenta) | emp::bold}, // Socket
            { "su", fmt::bg(tc::red) | emp::bold},     // SUID (u+s)
            { "sg", fmt::bg(tc::yellow) | emp::bold},  // SGID (g+s)
            // ca, tw, ow
            { "st", fmt::bg(tc::blue) },               // Sticky directory
            { "ex", fmt::fg(tc::green) },              // Executable
    };
}

struct DirInfo {
    fs::path name;
    std::vector<fs::directory_entry> entries;
    bool isError = false;

    explicit DirInfo(fs::path path) : name(std::move(path)) {
        std::error_code rc;
        auto iter = fs::directory_iterator(name, fs::directory_options::none, rc);
        if (!rc) {
            for (const auto &entry : iter) {
                entries.push_back(entry);
            }
            std::sort(entries.begin(), entries.end(), [](auto &l, auto &r) {
                return l.path().filename() < r.path().filename();
            });
        }
        else {
            isError = true;
        }
    }
};

static std::string colorizeDir(fs::path const &dir) {
    using tc = fmt::terminal_color;
    using emp = fmt::emphasis;
    // TODO: sticky => blue bg
    return fmt::format(fmt::fg(tc::blue) | emp::bold, "{}", dir);
}

static std::string colorizeFile(fs::path const &file) {
    using tc = fmt::terminal_color;
    using emp = fmt::emphasis;
    using pm = fs::perms;

    auto stat = fs::status(file);
    auto perms = stat.permissions();
    auto pmExecAll = pm::owner_exec | pm::group_exec | pm::others_exec;

    fmt::text_style style;
    if (fs::is_symlink(file)) {
        auto lnkDst = fs::read_symlink(file);
        if (fs::exists(lnkDst))
            style = fmt::fg(tc::cyan) | emp::bold;
        else
            style = fmt::fg(tc::red) | emp::bold;
    }
    else if (fs::is_directory(file)) {
        if ((perms & pm::sticky_bit) != pm::none)
            style = fmt::bg(tc::blue);
        else
            style = fmt::fg(tc::blue) | emp::bold;
    }
    else if (fs::is_regular_file(file)) {
        if (fs::is_block_file(file) || fs::is_character_file(file))
            style = fmt::fg(tc::yellow) | emp::bold;
        else if (fs::is_fifo(file))
            style = fmt::fg(tc::yellow);
        else if (fs::is_socket(file))
            style = fmt::fg(tc::magenta) | emp::bold;
        else {
            if ((perms & pmExecAll) != pm::none)
                style = fmt::fg(tc::green) | emp::bold;
        }
    }
    // TODO: suid, sgid, sow, ow, missing
    return fmt::format(style, "{}", file.filename());
}

static void walkDirectory(DirInfo const &dir, std::string const &prefix) {
    using namespace std::string_literals; // │├─┬└
    const std::array prefMid { "├─ "s, "│  "s };
    const std::array prefLast { "└─ "s, "   "s };

    size_t i = 0;
    std::string turnStr = prefix + prefMid[0];
    std::string skipStr = prefix + prefMid[1];
    for (const auto &entry : dir.entries) {
        i++;
        bool isLast = (i == dir.entries.size());
        if (isLast) {
            turnStr.erase(turnStr.size() - prefMid[0].size());
            turnStr.append(prefLast[0]);
        }

        fmt::print("{}", turnStr);

        if (fs::is_symlink(entry)) {
            auto dst = fs::read_symlink(entry.path());
            fmt::print("{} -> {}\n", colorizeFile(entry), colorizeFile(dst));
        }
        else if (fs::is_directory(entry)) {
            if (isLast) {
                skipStr.erase(skipStr.size() - prefMid[1].size());
                skipStr.append(prefLast[1]);
            }

            DirInfo nestedDir(entry);
            while (nestedDir.entries.size() == 1 && nestedDir.entries[0].is_directory()) {
                fmt::print("{}{}", colorizeFile(nestedDir.name),
                           fs::path::preferred_separator);
                nestedDir = DirInfo(nestedDir.entries[0]);
            }

            if (!nestedDir.isError) {
                fmt::print("{}{}\n", colorizeFile(nestedDir.name),
                           fs::path::preferred_separator);
                walkDirectory(nestedDir, skipStr);
            }
            else {
                fmt::print("{} [error opening dir]\n", colorizeFile(entry));
            }
        }
        else {
            fmt::print("{}\n", colorizeFile(entry));
        }
    }
}

int main(int argc, char **argv) {
    fs::path rootPath = argc <= 1 ? "./" : argv[1];
    if (!fs::exists(rootPath)) {
        fmt::print("File '{}' doesn't exist\n", rootPath);
        return EXIT_FAILURE;
    }

    if (fs::is_directory(rootPath)) {
        DirInfo dinfo(rootPath);
        if (!dinfo.isError) {
            fmt::print("{}\n", colorizeDir(rootPath));
            std::string prefix;
            walkDirectory(dinfo, prefix);
        }
        else {
            fmt::print("{} [error opening dir]\n", colorizeDir(rootPath));
        }
    }
    else {
        fmt::print("{}\n", colorizeFile(rootPath));
    }

    return EXIT_SUCCESS;
}
