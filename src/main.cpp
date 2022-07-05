#include <vector>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/color.h>
#include "util.hpp"

namespace fs = std::filesystem;

enum class FileColorType {
    REGULAR, DIRECTORY, SYMLINK,
    FIFO, BLOCKDEV, CHARDEV, SOCKET,
    ORPHAN, MISSING,
    SUID, SGID, STICKY_DIR,
    EXECUTABLE, CAPABILITY,
    STICKY_OTH_WR, OTHERS_WR,
    DOOR,
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
static fmt::text_style getStyle(FileColorType type) {
    using tc = fmt::terminal_color;
    using emp = fmt::emphasis;

    switch (type) {
        case FileColorType::REGULAR: // "fi"
            return fmt::text_style();
        case FileColorType::DIRECTORY: // "di" '/'
            return fmt::fg(tc::blue) | emp::bold;
        case FileColorType::SYMLINK: // "ln" '@'
            return fmt::fg(tc::cyan) | emp::bold;
        case FileColorType::FIFO: // "pi" '|'
            return fmt::fg(tc::yellow);
        case FileColorType::BLOCKDEV: // "bd"
        case FileColorType::CHARDEV: // "cd"
            return fmt::fg(tc::yellow) | emp::bold;
        case FileColorType::ORPHAN: // "or"
            return fmt::fg(tc::red) | emp::bold;
        case FileColorType::MISSING: // "mi"
            return fmt::text_style();
        case FileColorType::SOCKET: // "so" '='
        case FileColorType::DOOR: // "do" '>'
            return fmt::fg(tc::magenta) | emp::bold;
        case FileColorType::SUID: // "su"
            return fmt::bg(tc::red);
        case FileColorType::SGID: // "sg"
            return fmt::fg(tc::black) | fmt::bg(tc::yellow);
        case FileColorType::STICKY_DIR: // "st"
            return fmt::bg(tc::blue);
        case FileColorType::EXECUTABLE: // "ex" '*'
            return fmt::fg(tc::green) | emp::bold;

        case FileColorType::CAPABILITY: // "ca"
            return fmt::fg(tc::black) | fmt::bg(tc::red);
        case FileColorType::STICKY_OTH_WR: // "tw"
            return fmt::fg(tc::black) | fmt::bg(tc::green);
        case FileColorType::OTHERS_WR: // "ow"
            return fmt::fg(tc::blue) | fmt::bg(tc::green);
    }
    return fmt::text_style();
}
#pragma clang diagnostic pop

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

static std::string colorizeFile(fs::path const &file, std::string const &text = "") {
    using pm = fs::perms;
    using col = FileColorType;

    auto stat = fs::status(file);
    auto perms = stat.permissions();

    fmt::text_style style = getStyle(col::REGULAR);
    if (fs::is_symlink(file)) {
        auto lnkDst = fs::read_symlink(file);
        if (lnkDst.is_relative())
            lnkDst = file.parent_path() / lnkDst;
        if (fs::exists(lnkDst))
            style = getStyle(col::SYMLINK);
        else
            style = getStyle(col::ORPHAN);
    }
    else if (fs::is_directory(file)) {
        if ((perms & pm::sticky_bit) != pm::none)
            style = getStyle(col::STICKY_DIR);
        else
            style = getStyle(col::DIRECTORY);
    }
    else if (fs::is_block_file(file))
        style = getStyle(col::BLOCKDEV);
    else if (fs::is_character_file(file))
        style = getStyle(col::CHARDEV);
    else if (fs::is_fifo(file))
        style = getStyle(col::FIFO);
    else if (fs::is_socket(file))
        style = getStyle(col::SOCKET);
    else if (fs::is_regular_file(file)) {
        auto pmExecAll = pm::owner_exec | pm::group_exec | pm::others_exec;
        if ((perms & pm::set_uid) != pm::none)
            style = getStyle(col::SUID);
        else if ((perms & pm::set_gid) != pm::none)
            style = getStyle(col::SGID);
#if 0
        else if ((perms & pm::others_write) != pm::none) {
            if ((perms & pm::sticky_bit) != pm::none)
                style = getStyle(col::STICKY_OTH_WR);
            else
                style = getStyle(col::OTHERS_WR);
        }
#endif
        else if ((perms & pmExecAll) != pm::none)
            style = getStyle(col::EXECUTABLE);
    }
    else if (!fs::exists(file))
        style = getStyle(col::MISSING);

    if (text.empty())
        return fmt::format(style, "{}", file.filename());
    else
        return fmt::format(style, "{}", text);
}

static void walkDirectory(DirInfo const &dir, std::string const &prefix) {
    using namespace std::string_literals; // ┬
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
            fs::path dst = fs::read_symlink(entry.path());
            fs::path absDst = dst;
            if (absDst.is_relative())
                absDst = entry.path().parent_path() / absDst;
            fmt::print("{} -> {}\n", colorizeFile(entry),
                       colorizeFile(absDst, dst));
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

static void processRoot(fs::path const &root) {
    if (fs::is_directory(root)) {
        DirInfo dinfo(root);
        if (!dinfo.isError) {
            fmt::print("{}\n", colorizeFile(root, root));
            std::string prefix;
            walkDirectory(dinfo, prefix);
        }
        else {
            fmt::print("{} [error opening dir]\n",
                       colorizeFile(root, root));
        }
    }
    else if (fs::is_symlink(root)) {
        fs::path dst = fs::read_symlink(root);
        fs::path absDst = dst;
        if (absDst.is_relative())
            absDst = root.parent_path() / absDst;
        fmt::print("{} -> {}\n", colorizeFile(root, root),
                   colorizeFile(absDst, dst));
    }
    else {
        fmt::print("{}\n", colorizeFile(root, root));
    }
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        processRoot("./");
        return EXIT_SUCCESS;
    }

    for (int i = 1; i < argc; i++) {
        fs::path rootPath = argv[i];
        if (!fs::exists(rootPath)) {
            fmt::print("File '{}' doesn't exist\n", rootPath);
            return EXIT_FAILURE;
        }
        processRoot(rootPath);
    }
    return EXIT_SUCCESS;
}
