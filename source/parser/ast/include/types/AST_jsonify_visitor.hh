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

#ifndef __AST_JSONIFY_VISIT_H__
#define __AST_JSONIFY_VISIT_H__

#include "neo-json/include/json.hh"
#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/nodes/AST_Nodes.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "parser/ast/include/types/AST_visitor.hh"

__AST_VISITOR_BEGIN {
    class Jsonify : public Visitor {
      public:
        Jsonify()                           = default;
        Jsonify(const Jsonify &)            = default;
        Jsonify(Jsonify &&)                 = delete;
        Jsonify &operator=(const Jsonify &) = default;
        Jsonify &operator=(Jsonify &&)      = delete;
        ~Jsonify() override                 = default;

        neo::json json{"ast"};

        GENERATE_VISIT_EXTENDS;
    };

    inline neo::json get_node_json(const NodeT<> &node) {
        auto visitor = Jsonify();
        node->accept(visitor);
        return visitor.json;
    }
}  // namespace __AST_BEGIN

#endif  // __AST_JSONIFY_VISIT_H__