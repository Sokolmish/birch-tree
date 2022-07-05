#ifndef FILE_HPP_INCLUDED__
#define FILE_HPP_INCLUDED__

#include <vector>
#include <optional>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

class FileInfo {
public:
    fs::path path;

    explicit FileInfo(fs::path path) : path(std::move(path)) {}
    FileInfo(fs::path path, fs::file_status stat) : path(std::move(path)), stat(stat) {}

    fs::file_status getSymStat() {
        if (!stat.has_value())
            stat = fs::symlink_status(path);
        return stat.value();
    }

private:
    std::optional<fs::file_status> stat = std::nullopt;
};

class DirInfo final : public FileInfo {
public:
    std::vector<fs::directory_entry> entries;
    bool isError = false;

    explicit DirInfo(fs::path dirPath) : FileInfo(std::move(dirPath)) {
        readContent();
    }

    explicit DirInfo(FileInfo file) : FileInfo(std::move(file)) {
        readContent();
    }

private:
    void readContent() {
        std::error_code rc;
        auto iter = fs::directory_iterator(path, fs::directory_options::none, rc);
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

#endif /* FILE_HPP_INCLUDED__ */
