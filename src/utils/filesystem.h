#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace Filesystem
{
    // Throws if the directory can't be accessed.
    // The list might be sorted, but we shouldn't rely on that.
    // Expect the list to contain `.` and `..`.
    std::vector<std::string> GetDirectoryContents(const std::string &dir_name);

    enum EntryCategory {file, directory, other}; // Symlinks can't be detected due to MinGW-w64 `sys/stat.h` limitations.

    struct EntryInfo
    {
        EntryCategory category;
        std::time_t time_modified; // Modification of files in nested directories doesn't affect this time.
    };

    // Throws if the file or directory can't be accessed.
    EntryInfo GetEntryInfo(const std::string &entry_name);

    struct TreeNode
    {
        std::string name; // File name without path. For the root node returned by `GetEntryTree` this is equal to `entry_name`.
        std::string path; // Starts with `entry_name` passed to `GetEntryTree` and ends with `name`.
        EntryInfo info;
        std::size_t time_modified_recursive; // Unlike `info.time_modified` this takes into account the directory contents. For files they are equal.
        std::vector<TreeNode> contents;
    };

    // Using a negative `max_depth` disables depth limit. But then a circular symlink might cause stack overflow.
    TreeNode GetEntryTree(const std::string &entry_name, int max_depth = 64);
}