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

#ifndef __DEPENDENCY_TREE_H__
#define __DEPENDENCY_TREE_H__

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "neo-pprint/include/hxpprint.hh"

namespace parser::preprocessor {
using ImportNodePtr  = std::shared_ptr<struct ImportNode>;
using ImportNodeUPtr = std::unique_ptr<class ImportTree>;

struct ImportNode {
    using ImportNodes       = std::vector<ImportNodePtr>;
    using ImportNodeWeekPtr = std::weak_ptr<ImportNode>;

    std::string       module_name;
    ImportNodes       children;
    ImportNodeWeekPtr parent;

    explicit ImportNode(std::string name, const ImportNodePtr &parent_node = nullptr)
        : module_name(std::move(name))
        , parent(parent_node) {}

    ~ImportNode()                                  = default;
    ImportNode(ImportNode &&other)                 = default;
    ImportNode(const ImportNode &other)            = delete;
    ImportNode &operator=(ImportNode &&other)      = delete;
    ImportNode &operator=(const ImportNode &other) = delete;
};

class ImportTree {
  public:
    explicit ImportTree(std::string name)
        : root(std::make_shared<ImportNode>(std::move(name))) {}

    ~ImportTree()                                  = default;
    ImportTree(ImportTree &&other)                 = default;
    ImportTree(const ImportTree &other)            = delete;
    ImportTree &operator=(ImportTree &&other)      = delete;
    ImportTree &operator=(const ImportTree &other) = delete;

    ImportNodePtr add_import(const std::string &module_name, ImportNodePtr parent = nullptr) {
        auto node = std::make_shared<ImportNode>(module_name, parent);
        if (parent != nullptr) {
            parent->children.push_back(node);
        } else {
            root->children.push_back(node);
        }
        return node;
    }

    void print_tree(const ImportNodePtr &node,
                    const std::string   &prefix  = "",
                    bool                 is_last = true,
                    int                  depth   = 0) {
        if (node == nullptr) {
            return;
        }

        // Define ANSI color codes
        const std::string RESET   = "\033[0m";
        const std::string CYAN    = "\033[36m";
        const std::string MAGENTA = "\033[35m";
        const std::string BLUE    = "\033[34m";
        const std::string YELLOW  = "\033[33m";

        print(prefix);

        if (depth != 0) {
            print((is_last ? "└── " : "├── "));
        }

        print((depth == 0 ? CYAN : (depth % 2 == 0 ? MAGENTA : BLUE)),
              node->module_name,
              RESET,
              '\n');

        std::string new_prefix =
            prefix + (is_last ? std::string(static_cast<int>(depth * 4), ' ') : "│   ");

        for (size_t i = 0; i < node->children.size(); ++i) {
            print_tree(node->children[i], new_prefix, i == node->children.size() - 1, depth + 1);
        }
    }

    ImportNodePtr get_root() { return root; }

  private:
    ImportNodePtr root;
};

inline ImportNodeUPtr import_tree;

}  //  namespace parser::preprocessor

#endif  // __DEPENDENCY_TREE_H__