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
#include "derplanner/compiler/term.h"
#include "derplanner/compiler/generic_node_ops.h"
#include "derplanner/compiler/logical_expression.h"

namespace plnnrc {
namespace ast {

namespace
{
    const char token_and[] = "and";
    const char token_or[]  = "or";
    const char token_not[] = "not";

    // forward
    node* build_recursive(tree& ast, sexpr::node* s_expr);

    node* build_logical_op(tree& ast, sexpr::node* s_expr, node_type op_type)
    {
        node* root = ast.make_node(op_type, s_expr);

        if (!root)
        {
            return 0;
        }

        plnnrc_assert(root != 0);
        plnnrc_assert(s_expr->first_child != 0);

        for (sexpr::node* c_expr = s_expr->first_child->next_sibling; c_expr != 0; c_expr = c_expr->next_sibling)
        {
            node* child = build_recursive(ast, c_expr);

            if (!child)
            {
                return 0;
            }

            append_child(root, child);
        }

        return root;
    }

    node* build_recursive(tree& ast, sexpr::node* s_expr)
    {
        plnnrc_assert(s_expr->type == sexpr::node_list);
        sexpr::node* c_expr = s_expr->first_child;
        plnnrc_assert(c_expr && c_expr->type == sexpr::node_symbol);

        if (strncmp(c_expr->token, token_and, sizeof(token_and)) == 0)
        {
            return build_logical_op(ast, s_expr, node_op_and);
        }

        if (strncmp(c_expr->token, token_or, sizeof(token_or)) == 0)
        {
            return build_logical_op(ast, s_expr, node_op_or);
        }

        if (strncmp(c_expr->token, token_not, sizeof(token_not)) == 0)
        {
            return build_logical_op(ast, s_expr, node_op_not);
        }

        return build_atom(ast, s_expr);
    }
}

node* build_logical_expression(tree& ast, sexpr::node* s_expr)
{
    plnnrc_assert(s_expr->type == sexpr::node_list);

    node* root = ast.make_node(node_op_and, s_expr);

    if (!root)
    {
        return 0;
    }

    for (sexpr::node* c_expr = s_expr->first_child; c_expr != 0; c_expr = c_expr->next_sibling)
    {
        node* child = build_recursive(ast, c_expr);

        if (!child)
        {
            return 0;
        }

        append_child(root, child);
    }

    return root;
}

node* convert_to_nnf(tree& ast, node* root)
{
    node* r = root;

    for (node* p = root; p != 0; p = preorder_traversal_next(r, p))
    {
        if (p->type == node_op_not)
        {
            node* c = p->first_child;
            plnnrc_assert(c != 0);

            if (is_logical_op(c))
            {
                // eliminate double negation: (not (not x)) = (x)
                if (c->type == node_op_not)
                {
                    node* x = c->first_child;
                    plnnrc_assert(x != 0);
                    detach_node(x);

                    // get rid of 'p' and 'c'
                    if (p->parent)
                    {
                        insert_child(p, x);
                        detach_node(p);
                    }

                    // update root of the tree if we're replacing it
                    if (r == p)
                    {
                        r = x;
                    }

                    p = x;
                }
                // de-morgan's law: (not (or x y)) = (and (not x) (not y)); (not (and x y)) = (or (not x) (not y))
                else
                {
                    p->type = (c->type == node_op_and) ? node_op_or : node_op_and;
                    node* after = c;

                    for (node* x = c->first_child; x != 0;)
                    {
                        node* next_x = x->next_sibling;

                        detach_node(x);

                        node* new_not = ast.make_node(node_op_not);

                        if (!new_not)
                        {
                            return 0;
                        }

                        append_child(new_not, x);
                        insert_child(after, new_not);
                        after = new_not;

                        x = next_x;
                    }

                    detach_node(c);
                }
            }
        }
    }

    return r;
}

void flatten(node* root)
{
    plnnrc_assert(root != 0);

    for (node* p = root; p != 0; p = preorder_traversal_next(root, p))
    {
        if (is_logical_op(p) && p->type != node_op_not)
        {
            while (true)
            {
                bool collapsed = false;

                for (node* n = p->first_child; n != 0;)
                {
                    node* next_n = n->next_sibling;

                    // collapse: (and x (and y) z) -> (and x y z); (or x (or y) z) -> (or x y z)
                    if (n->type == p->type)
                    {
                        node* after = n;

                        for (node* c = n->first_child; c != 0;)
                        {
                            node* next_c = c->next_sibling;
                            detach_node(c);
                            insert_child(after, c);
                            after = c;
                            c = next_c;
                        }

                        detach_node(n);
                        collapsed = true;
                    }

                    n = next_n;
                }

                if (!collapsed)
                {
                    break;
                }
            }
        }
    }
}

namespace
{
    inline bool is_literal(node* root)
    {
        plnnrc_assert(root != 0);
        return (root->type == node_atom) || (root->type == node_op_not);
    }

    bool is_conjunction_of_literals(node* root)
    {
        plnnrc_assert(root != 0);

        if (root->type != node_op_and)
        {
            return false;
        }

        for (node* n = root->first_child; n != 0; n = n->next_sibling)
        {
            if (!is_literal(n))
            {
                return false;
            }
        }

        return true;
    }

    inline node* find_first(node* root, node_type type)
    {
        for (node* n = root->first_child; n != 0; n = n->next_sibling)
        {
            if (n->type == type)
            {
                return n;
            }
        }

        return 0;
    }

    bool distribute_and(tree& ast, node* node_and)
    {
        plnnrc_assert(node_and != 0);
        plnnrc_assert(node_and->type == node_op_and);

        node* node_or = find_first(node_and, node_op_or);
        plnnrc_assert(node_or != 0);

        node* after = node_and;

        for (node* or_child = node_or->first_child; or_child != 0;)
        {
            node* next_or_child = or_child->next_sibling;
            node* new_and = ast.make_node(node_op_and);

            if (!new_and)
            {
                return false;
            }

            for (node* and_child = node_and->first_child; and_child != 0;)
            {
                node* next_and_child = and_child->next_sibling;

                if (and_child != node_or)
                {
                    node* and_child_clone = ast.clone_subtree(and_child);

                    if (!and_child_clone)
                    {
                        return false;
                    }

                    append_child(new_and, and_child_clone);
                }
                else
                {
                    detach_node(or_child);
                    append_child(new_and, or_child);
                }

                and_child = next_and_child;
            }

            flatten(new_and);
            insert_child(after, new_and);
            after = new_and;

            or_child = next_or_child;
        }

        detach_node(node_and);

        return true;
    }

    node* convert_to_dnf_or(tree& ast, node* root)
    {
        plnnrc_assert(root != 0);
        plnnrc_assert(root->type == node_op_or);

        while (true)
        {
            bool done = true;

            for (node* n = root->first_child; n != 0;)
            {
                node* next_n = n->next_sibling;

                if (!is_literal(n) && !is_conjunction_of_literals(n))
                {
                    done = false;

                    if (!distribute_and(ast, n))
                    {
                        return 0;
                    }
                }

                n = next_n;
            }

            if (done)
            {
                break;
            }
        }

        return root;
    }
}

node* convert_to_dnf(tree& ast, node* root)
{
    plnnrc_assert(root);

    node* nnf_root = convert_to_nnf(ast, root);
    node* new_root = ast.make_node(node_op_or);

    if (!nnf_root || !new_root)
    {
        return 0;
    }

    append_child(new_root, nnf_root);
    flatten(new_root);

    return convert_to_dnf_or(ast, new_root);
}

}
}
