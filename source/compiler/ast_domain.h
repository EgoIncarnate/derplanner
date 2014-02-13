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

#ifndef DERPLANNER_COMPILER_DOMAIN_H_
#define DERPLANNER_COMPILER_DOMAIN_H_

namespace plnnrc {

namespace sexpr
{
    struct node;
}

namespace ast
{
    class tree;
}

namespace ast {

node* build_namespace(tree& ast, sexpr::node* s_expr);

node* build_domain(tree& ast, sexpr::node* s_expr);
node* build_method(tree& ast, sexpr::node* s_expr);
node* build_branch(tree& ast, sexpr::node* s_expr);
node* build_task_list(tree& ast, sexpr::node* s_expr);
node* build_operator(tree& ast, sexpr::node* s_expr);
node* build_operator_stub(tree& ast, sexpr::node* s_expr);
bool  build_operator_stubs(tree& ast);

node* build_worldstate(tree& ast, sexpr::node* s_expr);
node* build_worldstate_atom(tree& ast, sexpr::node* s_expr, int& type_tag);
node* build_worldstate_type(tree& ast, sexpr::node* s_expr, int& type_tag);

void infer_types(tree& ast);
void annotate(tree& ast);

}
}

#endif
