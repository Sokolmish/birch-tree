#include "birch_tree.hpp"
#include <filesystem>
#include <algorithm>
#include <fmt/format.h>
#include <fmt/color.h>


#define MSG_ERR_OPEN_DIR "[error opening dir]"
#define MSG_RECURSIVE_LNK "[recursive, not followed]"


// Needed for crossplatform isatty
#if defined(OS_LINUX) || defined(OS_MAC)
#include <unistd.h>
#elif defined(OS_WIN)
#include <windows.h>
#include <io.h>
#include <memory>
#endif

// Borrowed from https://github.com/agauniyal/rang/blob/master/include/rang.hpp
static bool isStdoutTerminal() {
#if defined(OS_LINUX) || defined(OS_MAC)
    static const bool isTerm = isatty(fileno(stdout)) != 0;
    return isTerm;
#elif defined(OS_WIN)
    static const bool isTerm =
            (_isatty(_fileno(stdout)) || isMsysPty(_fileno(stdout)));
    return isTerm;
#else
    return false;
#endif
}


void DirInfo::readContent() {
    std::error_code rc;
    auto iter = fs::directory_iterator(path, fs::directory_options::none, rc);
    if (!rc) {
        for (const auto &entry : iter) {
            entries.emplace_back(entry);
        }
    }
    else {
        isError = true;
    }
}


#ifdef __CLION_IDE__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
#endif // __CLION_IDE__

fmt::text_style BirchTree::getStyle(FileColorType type) const {
    using tc = fmt::terminal_color;
    using emp = fmt::emphasis;

    if (!opts.forceColor) {
        if (opts.noColor || !isStdoutTerminal())
            return fmt::text_style();
    }

    switch (type) {
        case FileColorType::REGULAR: // "fi"
            return fmt::text_style();
        case FileColorType::DIRECTORY: // "di"
            return fmt::fg(tc::blue) | emp::bold;
        case FileColorType::SYMLINK: // "ln"
            return fmt::fg(tc::cyan) | emp::bold;
        case FileColorType::FIFO: // "pi"
            return fmt::fg(tc::yellow);
        case FileColorType::BLOCKDEV: // "bd"
        case FileColorType::CHARDEV: // "cd"
            return fmt::fg(tc::yellow) | emp::bold;
        case FileColorType::ORPHAN: // "or"
            return fmt::fg(tc::red) | emp::bold;
        case FileColorType::MISSING: // "mi"
            return fmt::text_style();
        case FileColorType::SOCKET: // "so"
        case FileColorType::DOOR: // "do"
            return fmt::fg(tc::magenta) | emp::bold;
        case FileColorType::SUID: // "su"
            return fmt::bg(tc::red);
        case FileColorType::SGID: // "sg"
            return fmt::fg(tc::black) | fmt::bg(tc::yellow);
        case FileColorType::STICKY_DIR: // "st"
            return fmt::bg(tc::blue);
        case FileColorType::EXECUTABLE: // "ex"
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

char BirchTree::getFiletypeChar(FileColorType type) const {
    switch (type) {
        // case FileColorType::DIRECTORY: // Directories always have this sign anyway
        //    return '/';
        case FileColorType::SYMLINK:
            return '@';
        case FileColorType::FIFO:
            return '|';
        case FileColorType::SOCKET:
            return '=';
        case FileColorType::DOOR:
            return '>';
        case FileColorType::EXECUTABLE:
            return '*';
        default:
            return '\0';
    }
}

#ifdef __CLION_IDE__
#pragma clang diagnostic pop
#endif // __CLION_IDE__


std::string BirchTree::colorizeFile(FileInfo &file, std::string const &text = "") {
    using pm = fs::perms;

    fs::perms perms = file.getSymStat().permissions();

    FileColorType fType = FileColorType::REGULAR;
    if (fs::is_symlink(file.getSymStat())) {
        auto lnkDst = fs::read_symlink(file.getPath());
        if (lnkDst.is_relative())
            lnkDst = file.getPath().parent_path() / lnkDst;
        if (fs::exists(lnkDst))
            fType = FileColorType::SYMLINK;
        else
            fType = FileColorType::ORPHAN;
    }
    else if (fs::is_directory(file.getSymStat())) {
        if ((perms & pm::sticky_bit) != pm::none)
            fType = FileColorType::STICKY_DIR;
        else
            fType = FileColorType::DIRECTORY;
    }
    else if (fs::is_block_file(file.getSymStat()))
        fType = FileColorType::BLOCKDEV;
    else if (fs::is_character_file(file.getSymStat()))
        fType = FileColorType::CHARDEV;
    else if (fs::is_fifo(file.getSymStat()))
        fType = FileColorType::FIFO;
    else if (fs::is_socket(file.getSymStat()))
        fType = FileColorType::SOCKET;
    else if (fs::is_regular_file(file.getSymStat())) {
        auto pmExecAll = pm::owner_exec | pm::group_exec | pm::others_exec;
        if ((perms & pm::set_uid) != pm::none)
            fType = FileColorType::SUID;
        else if ((perms & pm::set_gid) != pm::none)
            fType = FileColorType::SGID;
#if 0
            else if ((perms & pm::others_write) != pm::none) {
            if ((perms & pm::sticky_bit) != pm::none)
                fType = FileColorType::STICKY_OTH_WR;
            else
                fType = FileColorType::OTHERS_WR;
        }
#endif
        else if ((perms & pmExecAll) != pm::none)
            fType = FileColorType::EXECUTABLE;
    }
    else if (!fs::exists(file.getSymStat()))
        fType = FileColorType::MISSING;

    fmt::text_style style = getStyle(fType);
    std::string wrText = text;
    if (wrText.empty())
        wrText = file.getPath().filename();
    std::string res = fmt::format(style, "{}", wrText);
    if (opts.filesSigns) {
        if (char sign = getFiletypeChar(fType))
            res += sign;
    }
    return res;
}

static bool isFileHidden(fs::path const &path) {
    return path.filename().string().at(0) == '.';
}

bool BirchTree::isDirCollapsable(DirInfo &dir) const {
    if (dir.entries.size() != 1)
        return false;
    auto &ent = dir.entries[0];
    if (!fs::is_directory(ent.getSymStat()))
        return false;
    if (!opts.showAll)
        return !isFileHidden(ent.getPath());
    return true;
}

void BirchTree::transformDirContent(std::vector<FileInfo> &files) const {
    if (!opts.unsorted) {
        std::sort(files.begin(), files.end(), [](auto &l, auto &r) {
            return l.getPath().filename() < r.getPath().filename();
        });

        if (opts.sortReverse) {
            std::reverse(files.begin(), files.end());
        }

        if (opts.dirsFirst) {
            std::stable_partition(files.begin(), files.end(),
                                  [](auto &x) { return fs::is_directory(x.getSymStat()); });
        }
    }

    if (!opts.showAll) {
        auto rm = std::remove_if(files.begin(), files.end(),
                                 [](auto &x) { return isFileHidden(x.getPath()); });
        files.erase(rm, files.end());
    }

    if (opts.dirsOnly) {
        auto rm = std::remove_if(files.begin(), files.end(),
                                 [](auto &x) { return !fs::is_directory(x.getSymStat()); });
        files.erase(rm, files.end());
    }
}

static FileInfo getSymlinksEnd(fs::path const &link) {
    fs::path absDst = fs::read_symlink(link);
    if (absDst.is_relative())
        absDst = link.parent_path() / absDst;
    FileInfo dst(absDst);

    while (dst.isSymlink()) {
        absDst = fs::read_symlink(dst.getPath());
        if (absDst.is_relative())
            absDst = dst.getPath().parent_path() / absDst;
        dst = FileInfo(absDst);
    }
    return dst;
}

static std::string getUniDirPath(fs::path const &dir) {
    // Remove separator after each path for unification
    std::string spath = (dir / "").lexically_normal().string();
    spath.erase(spath.size() - 1);
    return spath;
}

void BirchTree::walkDirectory(DirInfo &dir, std::string const &prefix, int depth) {
    using namespace std::string_literals; // ┬
    static const std::string turnMid ("├─ "s);
    static const std::string turnLast("└─ "s);
    static const std::string skipMid ("│  "s);
    static const std::string skipLast("   "s);

    if (opts.depth >= 0 && depth >= opts.depth)
        return;

    visitedDirs.emplace(getUniDirPath(dir.getPath()));

    transformDirContent(dir.entries);

    size_t i = 0;
    std::string turnStr = prefix + turnMid;
    std::string skipStr = prefix + skipMid;
    for (const auto &ent : dir.entries) {
        i++;
        bool isLast = (i == dir.entries.size());
        if (isLast) {
            turnStr.erase(turnStr.size() - turnMid.size());
            turnStr.append(turnLast);
            skipStr.erase(skipStr.size() - skipMid.size());
            skipStr.append(skipLast);
        }

        if (!opts.noIndent)
            fmt::print("{}", turnStr);

        FileInfo curFile(ent.getPath());
        if (fs::is_symlink(curFile.getSymStat())) {
            fs::path dst = fs::read_symlink(curFile.getPath());
            fs::path absDst = dst;
            if (absDst.is_relative())
                absDst = curFile.getPath().parent_path() / absDst;
            FileInfo dstFile(absDst);

            std::optional<DirInfo> dstDir;
            if (opts.followSymlinks) {
                if (dstFile.isDir()) {
                    dstDir = DirInfo(std::move(dstFile));
                }
                else if (dstFile.isSymlink()) {
                    FileInfo linksEnd = getSymlinksEnd(dstFile.getPath());
                    if (linksEnd.isDir()) {
                        dstDir = DirInfo(std::move(linksEnd));
                    }
                }
            }

            if (dstDir.has_value()) {
                dirsCnt++;
                if (visitedDirs.count(getUniDirPath(dstDir->getPath()))) {
                    fmt::print("{} -> {}  {}\n", colorizeFile(curFile),
                               colorizeFile(dstFile, dst), MSG_RECURSIVE_LNK);
                }
                else {
                    fmt::print("{} -> {}\n", colorizeFile(curFile),
                               colorizeFile(dstFile, dst));
                    walkDirectory(*dstDir, skipStr, depth + 1);
                }
            }
            else {
                // Note that in this case liks to directories will be counted as files
                filesCnt++;
                fmt::print("{} -> {}\n", colorizeFile(curFile),
                           colorizeFile(dstFile, dst));
            }
        }
        else if (fs::is_directory(curFile.getSymStat())) {
            DirInfo nestedDir(std::move(curFile));
            while (isDirCollapsable(nestedDir)) {
                dirsCnt++;
                fmt::print("{}{}", colorizeFile(nestedDir),
                           fs::path::preferred_separator);
                nestedDir = DirInfo(nestedDir.entries[0]);
            }

            if (!nestedDir.isError) {
                dirsCnt++;
                fmt::print("{}{}\n", colorizeFile(nestedDir),
                           fs::path::preferred_separator);
                walkDirectory(nestedDir, skipStr, depth + 1);
            }
            else {
                dirsCnt++;
                fmt::print("{}  {}\n", colorizeFile(nestedDir), MSG_ERR_OPEN_DIR);
            }
        }
        else { // Not a directory or symlink
            filesCnt++;
            fmt::print("{}\n", colorizeFile(curFile));
        }
    }
}

void BirchTree::processRoot(DirInfo &root) {
    if (fs::is_directory(root.getSymStat())) {
        if (!root.isError) {
            fmt::print("{}\n", colorizeFile(root, root.getPath()));
            std::string prefix;
            walkDirectory(root, prefix, 0);
        }
        else {
            fmt::print("{}  {}\n", colorizeFile(root, root.getPath()),
                       MSG_ERR_OPEN_DIR);
        }
    }
    else if (fs::is_symlink(root.getSymStat())) {
        fs::path dst = fs::read_symlink(root.getPath());
        fs::path absDst = dst;
        if (absDst.is_relative())
            absDst = root.getPath().parent_path() / absDst;
        FileInfo dstInfo(absDst);
        fmt::print("{} -> {}\n", colorizeFile(root, root.getPath()),
                   colorizeFile(dstInfo, dst));

        // Always follow top-level symlinks to directories
        if (fs::is_directory(dstInfo.getSymStat())) {
            DirInfo dstDir(std::move(dstInfo));
            std::string prefix;
            walkDirectory(dstDir, prefix, 0);
        }
    }
    else { // Not a directory or symlink
        fmt::print("{}\n", colorizeFile(root, root.getPath()));
    }
}
