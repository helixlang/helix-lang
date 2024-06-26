/**
 * @author Dhruvan Kartik
 * @copyright Copyright (c) 2024 (CC BY 4.0)
 *
 * @note This code is part of the Helix Language Project and is licensed under the Attribution 4.0
 * International license (CC BY 4.0). You are allowed to use, modify, redistribute, and create
 * derivative works, even for commercial purposes, provided that you give appropriate credit,
 * provide a link to the license, and indicate if changes were made. For more information, please
 * visit: https://creativecommons.org/licenses/by/4.0/ SPDX-License-Identifier: CC-BY-4.0
 *
 * @note This code is provided by the creators of Helix. Visit our website at:
 * https://helix-lang.com/ for more information.
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include <include/error/error.hh>
#include <tools/controllers/include/file_system.hh>

namespace file_system {
std::unordered_map<std::string, std::string> FileCache::cache_;
std::mutex FileCache::mutex_;

void FileCache::add_file(const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_[key] = value;
}

std::optional<std::string> FileCache::get_file(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto cache_it = cache_.find(key);
    if (cache_it != cache_.end()) {
        return cache_it->second;
    }
    return std::nullopt;
}

std::optional<std::string> get_line(const std::string &filename, u64 line) {
    std::string source = file_system::read_file(filename);

    u64 current_line = 1;
    u64 start = 0;
    u64 end = 0;

    for (u64 i = 0; i < source.size(); ++i) {
        if (source[i] == '\n') {
            ++current_line;
            if (current_line == line) {
                start = i + 1;
            } else if (current_line == line + 1) {
                end = i;
                break;
            }
        }

        if (i == source.size() - 1) {
            end = source.size();
        }
    }

    if (line > current_line) {
        return std::nullopt;
    }

    return source.substr(start, end - start);
}

std::string _internal_read_file(const std::string &filename) {
    auto cached_file = FileCache::get_file(filename);
    if (cached_file.has_value()) {
        return cached_file.value();
    }

    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file) {
        error::Error(error::Compiler{filename, "file not found."});
        return "";
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string source(size, '\0');
    if (!file.read(source.data(), size)) {
        error::Error(error::Compiler{filename, "failed to read file."});
        return "";
    }

    FileCache::add_file(filename, source);

    return source;
}

std::string read_file(std::string &filename) {
    std::optional<std::filesystem::path> path = file_system::resolve_path(filename);
    if (!path.has_value()) {
        error::Error(error::Compiler{filename, "file not found."});
        std::exit(1);
    }

    return _internal_read_file(path.value().string());
}

// TODO: make all readfiles after the first one be relative to the first one
/// Example: readfile("file1.hlx") -> /path/to/file1.hlx
///          readfile("test/file2.hlx") -> /path/to/test/file2.hlx
///          readfile("../file3.hlx") -> /path/to/file3.hlx

std::string read_file(const std::string &filename) {
    std::optional<std::filesystem::path> path = file_system::resolve_path(filename);
    if (!path.has_value()) {
        error::Error(error::Compiler{filename, "file not found."});
        std::exit(1);
    }

    return _internal_read_file(path.value().string());
}
}  // namespace file_system