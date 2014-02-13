//
// Copyright (c) 2013 Alexander Shafranov shafranov@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "derplanner/compiler/ast.h"
#include "tree_tools.h"
#include "ast_build_tools.h"

namespace plnnrc {

ast::node* report_error(ast::tree& ast, ast::node* parent, compilation_error error_id, sexpr::node* s_expr)
{
    ast::node* error = ast.make_node(ast::node_error, s_expr);

    if (error)
    {
        if (parent)
        {
            append_child(parent, error);

            if (ast.error_nodes.size() < ast.error_nodes.capacity())
            {
                ast.error_nodes.append(error);
            }
        }

        ast::annotation<ast::error_ann>(error)->id = error_id;
    }

    return error;
}

}
