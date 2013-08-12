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

#include <string.h>
#include "derplanner/compiler/assert.h"
#include "derplanner/compiler/s_expression.h"
#include "derplanner/compiler/ast.h"
#include "derplanner/compiler/logical_expression.h"
#include "derplanner/compiler/domain.h"

namespace plnnrc {
namespace ast {

namespace
{
    const char token_domain[] = ":domain";
    const char token_method[] = ":method";
}

node* build_domain(tree& t, sexpr::node* s_expr)
{
    plnnrc_assert(s_expr->type == sexpr::node_list);
    plnnrc_assert(s_expr->first_child);
    plnnrc_assert(s_expr->first_child->type == sexpr::node_symbol);
    plnnrc_assert(strncmp(s_expr->first_child->token, token_domain, sizeof(token_domain)) == 0);

    node* domain = t.make_node(node_domain, s_expr);

    if (!domain)
    {
        return 0;
    }

    for (sexpr::node* c_expr = s_expr->first_child->next_sibling; c_expr != 0; c_expr = c_expr->next_sibling)
    {
        node* method = build_method(t, c_expr);

        if (!method)
        {
            return 0;
        }

        append_child(domain, method);
    }

    return domain;
}

node* build_method(tree& t, sexpr::node* s_expr)
{
    plnnrc_assert(s_expr->type == sexpr::node_list);
    plnnrc_assert(s_expr->first_child);
    plnnrc_assert(s_expr->first_child->type == sexpr::node_symbol);
    plnnrc_assert(strncmp(s_expr->first_child->token, token_method, sizeof(token_method)) == 0);

    node* method = t.make_node(node_method, s_expr);

    if (!method)
    {
        return 0;
    }

    sexpr::node* task_atom_expr = s_expr->first_child->next_sibling;
    plnnrc_assert(task_atom_expr);
    plnnrc_assert(task_atom_expr->type == sexpr::node_list);

    node* task_atom = t.make_node(node_atom, task_atom_expr->first_child);

    if (!task_atom)
    {
        return 0;
    }

    for (sexpr::node* v_expr = task_atom_expr->first_child->next_sibling; v_expr != 0; v_expr = v_expr->next_sibling)
    {
        node* argument = t.make_node(node_term_variable, v_expr);

        if (!argument)
        {
            return 0;
        }

        append_child(task_atom, argument);
    }

    append_child(method, task_atom);

    sexpr::node* branch_precond_expr = task_atom_expr->next_sibling;

    while (branch_precond_expr)
    {
        node* branch = build_branch(t, branch_precond_expr);

        if (!branch)
        {
            return 0;
        }

        append_child(method, branch);

        branch_precond_expr = branch_precond_expr->next_sibling->next_sibling;
    }

    return method;
}

node* build_branch(tree& t, sexpr::node* s_expr)
{
    return 0;
}

}
}
