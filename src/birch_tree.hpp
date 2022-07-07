#ifndef BIRCH_TREE_HPP_INCLUDED__
#define BIRCH_TREE_HPP_INCLUDED__

#include <string>
#include <vector>
#include <unordered_set>
#include <optional>
#include <filesystem>
#include <cstdint>
#include <fmt/color.h>

#define BIRCH_TREE_VERSION "v0.0"


#if defined(__unix__) || defined(__unix) || defined(__linux__)
#define OS_LINUX
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#define OS_WIN
#elif defined(__APPLE__) || defined(__MACH__)
#define OS_MAC
#else
#warning "Unknown OS"
#endif


struct Options {
    bool showAll = false;          // -a, --all
    bool dirsOnly = false;         // -d
    bool followSymlinks = false;   // -l
    int depth = -1;                // -L
    bool noStats = false;          // --noreport

    bool filesSigns = false;       // -F
    bool noIndent = false;         // -i, --noindent
    bool noColor = false;          // -n, --nocolor
    bool forceColor = false;       // -C, --color

    bool unsorted = false;         // -U
    bool sortReverse = false;      // -r
    bool dirsFirst = false;        // --dirsfirst
};


namespace fs = std::filesystem;

class FileInfo {
public:
    explicit FileInfo(fs::path path) : path(std::move(path)) {}

    FileInfo(fs::path path, fs::file_status stat)
            : path(std::move(path)), stat(stat) {}

    [[nodiscard]] fs::path getPath() const {
        return path;
    }

    fs::file_status getSymStat() {
        if (!stat.has_value())
            stat = fs::symlink_status(path);
        return *stat;
    }

    [[nodiscard]] bool isDir() {
        return fs::is_directory(getSymStat());
    }

    [[nodiscard]] bool isSymlink() {
        return fs::is_symlink(getSymStat());
    }

protected:
    fs::path path;

private:
    std::optional<fs::file_status> stat = std::nullopt;
};

class DirInfo final : public FileInfo {
public:
    std::vector<FileInfo> entries;
    bool isError = false;

    explicit DirInfo(fs::path dirPath) : FileInfo(std::move(dirPath)) {
        readContent();
    }

    explicit DirInfo(FileInfo file) : FileInfo(std::move(file)) {
        readContent();
    }

private:
    void readContent();
};


class BirchTree {
public:
    explicit BirchTree(Options const &options) : opts(options) {}

    void processRoot(DirInfo &root);

    [[nodiscard]] size_t getDirsCnt() const {
        return dirsCnt;
    }

    [[nodiscard]] size_t getFilesCnt() const {
        return filesCnt;
    }

private:
    enum class FileColorType {
        REGULAR, DIRECTORY, SYMLINK,
        FIFO, BLOCKDEV, CHARDEV, SOCKET,
        ORPHAN, MISSING,
        SUID, SGID, STICKY_DIR,
        EXECUTABLE, CAPABILITY,
        STICKY_OTH_WR, OTHERS_WR,
        DOOR,
    };

    Options const &opts;

    std::unordered_set<std::string> visitedDirs;

    size_t dirsCnt = 0;
    size_t filesCnt = 0;

    void walkDirectory(DirInfo &dir, std::string const &prefix, int depth);

    void transformDirContent(std::vector<FileInfo> &files) const;

    [[nodiscard]] std::string colorizeFile(FileInfo &file, std::string const &text);

    [[nodiscard]] fmt::text_style getStyle(FileColorType type) const;
    [[nodiscard]] char getFiletypeChar(FileColorType type) const;

    [[nodiscard]] bool isDirCollapsable(DirInfo &dir) const;
};


#endif /* BIRCH_TREE_HPP_INCLUDED__ */
