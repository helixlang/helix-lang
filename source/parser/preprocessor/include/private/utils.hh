///--- The Helix Project ------------------------------------------------------------------------///
///                                                                                              ///
///   Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).    ///
///   You are allowed to use, modify, redistribute, and create derivative works, even for        ///
///   commercial purposes, provided that you give appropriate credit, and indicate if changes    ///
///   were made.                                                                                 ///
///                                                                                              ///
///   For more information on the license terms and requirements, please visit:                  ///
///     https://creativecommons.org/licenses/by/4.0/                                             ///
///                                                                                              ///
///   SPDX-License-Identifier: CC-BY-4.0                                                         ///
///   Copyright (c) 2024 The Helix Project (CC BY 4.0)                                           ///
///                                                                                              ///
///-------------------------------------------------------------------------------------- C++ ---///

#ifndef __FIND_P0_H__
#define __FIND_P0_H__

// helix_hdr -> 1
// helix_hdr_lib -> 2
// helix_mod -> 3
// helix_mod_lib -> 4
// invalid -> 0
#include <string>

inline int find_import_priority(bool is_module,  // NOLINT
                       bool found_helix_mod,
                       bool found_helix_hdr,
                       bool found_helix_mod_lib,
                       bool found_helix_hdr_lib) {
    if (is_module) {
        if (found_helix_mod && found_helix_mod_lib) {
            return 4;
        }

        if (found_helix_mod_lib) {
            return 4;
        }

        if (found_helix_mod) {
            return 3;
        }
    }

    if (found_helix_mod && found_helix_hdr && found_helix_mod_lib && found_helix_hdr_lib) {
        return 1;
    }

    if (found_helix_mod && found_helix_mod_lib && found_helix_hdr_lib) {
        return 2;
    }

    if (found_helix_mod && found_helix_hdr && found_helix_hdr_lib) {
        return 1;
    }

    if (found_helix_mod && found_helix_hdr && found_helix_mod_lib) {
        return 4;
    }

    if (found_helix_hdr && found_helix_mod_lib && found_helix_hdr_lib) {
        return 2;
    }

    if (found_helix_mod && found_helix_hdr) {
        return 1;
    }

    if (found_helix_mod && found_helix_mod_lib) {
        return 4;
    }

    if (found_helix_mod && found_helix_hdr_lib) {
        return 2;
    }

    if (found_helix_hdr && found_helix_mod_lib) {
        return 4;
    }

    if (found_helix_hdr && found_helix_hdr_lib) {
        return 2;
    }

    if (found_helix_mod_lib && found_helix_hdr_lib) {
        return 2;
    }

    if (found_helix_mod) {
        return 3;
    }

    if (found_helix_hdr) {
        return 1;
    }

    if (found_helix_mod_lib) {
        return 4;
    }

    if (found_helix_hdr_lib) {
        return 2;
    }

    return 0;
}

inline std::string sanitize_string(const std::string &input) {
    std::string output = "_";
    
    for (char ch : input) {
        if ((std::isalnum(ch) != 0) || ch == '_') {
            output += ch;
        } else {
            output += '_';
        }
    }
    
    return output + "_N";
}

#endif  // __FIND_P0_H__