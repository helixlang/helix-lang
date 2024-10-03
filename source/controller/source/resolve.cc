//===------------------------------------------ C++ ------------------------------------------====//
//                                                                                                //
//  Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).       //
//  You are allowed to use, modify, redistribute, and create derivative works, even for           //
//  commercial purposes, provided that you give appropriate credit, and indicate if changes       //
//   were made. For more information, please visit: https://creativecommons.org/licenses/by/4.0/  //
//                                                                                                //
//  SPDX-License-Identifier: CC-BY-4.0                                                            //
//  Copyright (c) 2024 (CC BY 4.0)                                                                //
//                                                                                                //
//====----------------------------------------------------------------------------------------====//

#include <filesystem>
#include <optional>
#include <string>

#include "controller/include/shared/file_system.hh"

__CONTROLLER_FS_BEGIN {
    std::optional<std::filesystem::path> resolve_path(const std::string &resolve) {
        std::filesystem::path path(resolve);

        if (!path.is_absolute()) {
            std::filesystem::path cwd = get_cwd();
            path                      = cwd / path;
            path                      = std::filesystem::absolute(path);
        }

        if (std::filesystem::exists(path)) {
            return path;
        }

        std::filesystem::path parent_path = path.parent_path();
        if (std::filesystem::exists(parent_path)) {
            return path;
        }

        // Path does not exist
        return std::nullopt;
    }

    std::optional<std::filesystem::path> resolve_path(const std::string &resolve,
                                                      const std::string &base) {
        std::filesystem::path base_path(base);

        if (!base_path.is_absolute()) {
            std::filesystem::path cwd = get_cwd();
            base_path                 = std::filesystem::absolute(cwd / base_path);
        }

        // if base_path is a file, use its parent directory
        if (std::filesystem::is_regular_file(base_path)) {
            base_path = base_path.parent_path();
        }

        // resolve resolve relative to base_path
        std::filesystem::path path = base_path / resolve;
        path                       = std::filesystem::absolute(path);

        if (std::filesystem::exists(path)) {
            return path;
        }

        return std::nullopt;
    }
}  // __CONTROLLER_FS_BEGIN