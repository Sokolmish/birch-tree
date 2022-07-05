#include <iostream>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/color.h>

#include "file.hpp"

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

static std::string colorizeFile(FileInfo &file, std::string const &text = "") {
    using pm = fs::perms;
    using col = FileColorType;

    fs::perms perms = file.getSymStat().permissions();

    fmt::text_style style = getStyle(col::REGULAR);
    if (fs::is_symlink(file.getSymStat())) {
        auto lnkDst = fs::read_symlink(file.path);
        if (lnkDst.is_relative())
            lnkDst = file.path.parent_path() / lnkDst;
        if (fs::exists(lnkDst))
            style = getStyle(col::SYMLINK);
        else
            style = getStyle(col::ORPHAN);
    }
    else if (fs::is_directory(file.getSymStat())) {
        if ((perms & pm::sticky_bit) != pm::none)
            style = getStyle(col::STICKY_DIR);
        else
            style = getStyle(col::DIRECTORY);
    }
    else if (fs::is_block_file(file.getSymStat()))
        style = getStyle(col::BLOCKDEV);
    else if (fs::is_character_file(file.getSymStat()))
        style = getStyle(col::CHARDEV);
    else if (fs::is_fifo(file.getSymStat()))
        style = getStyle(col::FIFO);
    else if (fs::is_socket(file.getSymStat()))
        style = getStyle(col::SOCKET);
    else if (fs::is_regular_file(file.getSymStat())) {
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
    else if (!fs::exists(file.getSymStat()))
        style = getStyle(col::MISSING);

    if (text.empty())
        return fmt::format(style, "{}", file.path.filename().string());
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
    for (const auto &ent : dir.entries) {
        i++;
        bool isLast = (i == dir.entries.size());
        if (isLast) {
            turnStr.erase(turnStr.size() - prefMid[0].size());
            turnStr.append(prefLast[0]);
        }

        fmt::print("{}", turnStr);

        FileInfo curFile(ent.path());

        if (fs::is_symlink(curFile.getSymStat())) {
            fs::path dst = fs::read_symlink(curFile.path);
            fs::path absDst = dst;
            if (absDst.is_relative())
                absDst = curFile.path.parent_path() / absDst;
            FileInfo dstFile(absDst);
            fmt::print("{} -> {}\n", colorizeFile(curFile),
                       colorizeFile(dstFile, dst));
        }
        else if (fs::is_directory(curFile.getSymStat())) {
            if (isLast) {
                skipStr.erase(skipStr.size() - prefMid[1].size());
                skipStr.append(prefLast[1]);
            }

            DirInfo nestedDir(std::move(curFile));
            while (nestedDir.entries.size() == 1 && nestedDir.entries[0].is_directory()) {
                fmt::print("{}{}", colorizeFile(nestedDir),
                           fs::path::preferred_separator);
                nestedDir = DirInfo(nestedDir.entries[0]);
            }

            if (!nestedDir.isError) {
                fmt::print("{}{}\n", colorizeFile(nestedDir),
                           fs::path::preferred_separator);
                walkDirectory(nestedDir, skipStr);
            }
            else {
                fmt::print("{} [error opening dir]\n", colorizeFile(nestedDir));
            }
        }
        else {
            fmt::print("{}\n", colorizeFile(curFile));
        }
    }
}

static void processRoot(DirInfo &root) {
    if (fs::is_directory(root.getSymStat())) {
        if (!root.isError) {
            fmt::print("{}\n", colorizeFile(root, root.path));
            std::string prefix;
            walkDirectory(root, prefix);
        }
        else {
            fmt::print("{} [error opening dir]\n",
                       colorizeFile(root, root.path));
        }
    }
    else if (fs::is_symlink(root.getSymStat())) {
        fs::path dst = fs::read_symlink(root.path);
        fs::path absDst = dst;
        if (absDst.is_relative())
            absDst = root.path.parent_path() / absDst;
        FileInfo dstInfo(absDst);
        fmt::print("{} -> {}\n", colorizeFile(root, root.path),
                   colorizeFile(dstInfo, dst));

        // Always follow top-level symlinks to directories
        if (fs::is_directory(dstInfo.getSymStat())) {
            DirInfo dstDir(std::move(dstInfo));
            std::string prefix;
            walkDirectory(dstDir, prefix);
        }
    }
    else {
        fmt::print("{}\n", colorizeFile(root, root.path));
    }
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        DirInfo curDir("./");
        processRoot(curDir);
        return EXIT_SUCCESS;
    }

    for (int i = 1; i < argc; i++) {
        DirInfo root(argv[i]);
        if (!fs::exists(root.getSymStat())) {
            fmt::print("File '{}' doesn't exist\n", root.path.string());
            return EXIT_FAILURE;
        }
        processRoot(root);
    }
    return EXIT_SUCCESS;
}
