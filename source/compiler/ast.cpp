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

#include <string.h> // memset, memcpy
#include <stdint.h> // unitptr_t
#include "pool.h"
#include "derplanner/compiler/config.h"
#include "derplanner/compiler/assert.h"
#include "derplanner/compiler/memory.h"
#include "derplanner/compiler/tree_ops.h"
#include "derplanner/compiler/ast.h"

namespace plnnrc {
namespace ast {

namespace
{
    const size_t node_page_size = DERPLANNER_AST_NODE_MEMPAGE_SIZE;
    const size_t token_page_size = DERPLANNER_AST_TOKEN_MEMPAGE_SIZE;

    struct annotation_trait
    {
        size_t size;
        size_t alignment;
    };

    annotation_trait type_annotation_trait(node_type type)
    {
        annotation_trait result = {0};

        if (is_term(type))
        {
            result.size = sizeof(term_ann);
            result.alignment = plnnrc_alignof(term_ann);
            return result;
        }

        if (type == node_worldstate_type)
        {
            result.size = sizeof(ws_type_ann);
            result.alignment = plnnrc_alignof(ws_type_ann);
            return result;
        }

        if (type == node_atom)
        {
            result.size = sizeof(atom_ann);
            result.alignment = plnnrc_alignof(atom_ann);
            return result;
        }

        return result;
    }
}

tree::tree()
    : _node_pool(0)
    , _token_pool(0)
{
    memset(&_root, 0, sizeof(_root));
    _root.type = node_domain;
}

tree::~tree()
{
    if (_node_pool)
    {
        pool::clear(_node_pool);
    }

    if (_token_pool)
    {
        pool::clear(_token_pool);
    }
}

node* tree::make_node(node_type type, sexpr::node* token)
{
    if (!_node_pool)
    {
        pool::handle* pool = pool::init(node_page_size);

        if (!pool)
        {
            return 0;
        }

        _node_pool = pool;
    }

    node* n = static_cast<node*>(pool::allocate(_node_pool, sizeof(node), plnnrc_alignof(node)));

    if (n)
    {
        n->type = type;
        n->s_expr = token;
        n->parent = 0;
        n->first_child = 0;
        n->next_sibling = 0;
        n->prev_sibling_cyclic = 0;
        n->annotation = 0;

        annotation_trait t = type_annotation_trait(type);

        if (t.size > 0)
        {
            n->annotation = pool::allocate(_node_pool, t.size, t.alignment);

            if (!n->annotation)
            {
                return 0;
            }

            memset(n->annotation, 0, t.size);
        }
    }

    return n;
}

char* tree::make_token(size_t length)
{
    plnnrc_assert(length > 0);

    if (!_token_pool)
    {
        pool::handle* pool = pool::init(token_page_size);

        if (!pool)
        {
            return 0;
        }

        _token_pool = pool;
    }

    char* t = static_cast<char*>(pool::allocate(_token_pool, length+1, sizeof(void*)));

    memset(t, 0, length+1);

    return t;
}

node* tree::clone_node(node* original)
{
    plnnrc_assert(original != 0);
    node* n = make_node(original->type, original->s_expr);

    if (n && n->annotation)
    {
        memcpy(n->annotation, original->annotation, type_annotation_trait(original->type).size);
    }

    return n;
}

node* tree::clone_subtree(node* original)
{
    plnnrc_assert(original != 0);

    node* clone = clone_node(original);

    if (!clone)
    {
        return 0;
    }

    for (node* child_original = original->first_child; child_original != 0; child_original = child_original->next_sibling)
    {
        node* child_clone = clone_subtree(child_original);

        if (!child_clone)
        {
            return 0;
        }

        append_child(clone, child_clone);
    }

    return clone;
}

}
}
