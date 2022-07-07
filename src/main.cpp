#include "birch_tree.hpp"

#include <vector>
#include <string>
#include <fmt/format.h>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

int main(int argc, char **argv) {
    CLI::App app("An alternative to standard tree utility", "birch-tree");
#if defined(OS_WIN) || defined(BIRCHTREE_ALLOW_WIN_OPTS)
    app.allow_windows_style_options();
#endif
    app.allow_extras();

    Options opts;
    bool showVersion = false;
    std::vector<std::string> files;

    app.add_flag("--version", showVersion, "Print version and exit");

    std::string filesOptsGroupName = "Files options";
    app.add_flag("-a,--all", opts.showAll, "Do not skip hidden files")
            ->group(filesOptsGroupName);
    app.add_flag("-d", opts.dirsOnly, "Show only directories")
            ->group(filesOptsGroupName);
    app.add_flag("-l", opts.followSymlinks, "Follow symlinks")
            ->group(filesOptsGroupName);
    app.add_option("-L", opts.depth, "Set maximum directories depth")
            ->group(filesOptsGroupName)
            ->check(CLI::PositiveNumber);
    app.add_flag("--noreport", opts.noStats, "Disable files counters")
            ->group(filesOptsGroupName);

    std::string formatOptsGroupName = "Output format options";
    app.add_flag("-F", opts.filesSigns, "Append files signs as in ls -F")
            ->group(formatOptsGroupName);
    app.add_flag("-i,--noindent", opts.noIndent, "Do not print indentations")
            ->group(formatOptsGroupName);
    app.add_flag("-n,--nocolor", opts.noColor, "Disable colorization")
            ->group(formatOptsGroupName);
    app.add_flag("-C,--color", opts.forceColor, "Force colorization")
            ->group(formatOptsGroupName);

    std::string sortOptsGroupName = "Sorting options";
    app.add_flag("-U", opts.unsorted, "Leave files unsorted")
            ->group(sortOptsGroupName);
    app.add_flag("-r", opts.sortReverse, "Reverse sorting order")
            ->group(sortOptsGroupName);
    app.add_flag("--dirsfirst", opts.dirsFirst, "List directories before files")
            ->group(sortOptsGroupName);

    app.add_option("dirs", files, "Show trees for these directories");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (showVersion) {
        fmt::print("birch-tree {}\n"
                   "Made in 2022 by Mikhail Sokolovskiy\n"
                   "https://github.com/Sokolmish/birch-tree\n",
                   BIRCH_TREE_VERSION);
        return EXIT_SUCCESS;
    }

    std::vector<std::string> remaining = app.remaining();
    files.reserve(files.size() + remaining.size());
    files.insert(files.end(), remaining.begin(), remaining.end());

    if (files.empty())
        files.emplace_back("./");

    BirchTree birchTree(opts);

    for (std::string const &filename : files) {
        DirInfo root(filename);
        if (!fs::exists(root.getSymStat())) {
            fmt::print("File '{}' doesn't exist\n", root.getPath().string());
            return EXIT_FAILURE;
        }
        birchTree.processRoot(root);
    }

    if (!opts.noStats) {
        if (opts.dirsOnly) {
            fmt::print("\n{} directories\n", birchTree.getDirsCnt());
        }
        else {
            fmt::print("\n{} directories, {} files\n", birchTree.getDirsCnt(),
                       birchTree.getFilesCnt());
        }
    }

    return EXIT_SUCCESS;
}
