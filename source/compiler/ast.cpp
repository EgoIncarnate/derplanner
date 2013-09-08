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

#include <stddef.h> // size_t
#include <string.h> // memset, memcpy
#include <stdint.h> // unitptr_t
#include "pool.h"
#include "derplanner/compiler/assert.h"
#include "derplanner/compiler/memory.h"
#include "derplanner/compiler/ast.h"

namespace plnnrc {
namespace ast {

namespace
{
    const size_t page_size = 64 * 1024;

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
    : _pool(0)
{
    memset(&_root, 0, sizeof(_root));
    _root.type = node_domain;
}

tree::~tree()
{
    if (_pool)
    {
        pool::clear(_pool);
    }
}

node* tree::make_node(node_type type, sexpr::node* token)
{
    if (!_pool)
    {
        pool::handle* pool = pool::init(page_size);

        if (!pool)
        {
            return 0;
        }

        _pool = pool;
    }

    node* n = static_cast<node*>(pool::allocate(_pool, sizeof(node), plnnrc_alignof(node)));

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
            n->annotation = pool::allocate(_pool, t.size, t.alignment);

            if (!n->annotation)
            {
                return 0;
            }

            memset(n->annotation, 0, t.size);
        }
    }

    return n;
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

void append_child(node* parent, node* child)
{
    plnnrc_assert(parent != 0);
    plnnrc_assert(child != 0);

    child->parent = parent;
    child->prev_sibling_cyclic = 0;
    child->next_sibling = 0;

    node* first_child = parent->first_child;

    if (first_child)
    {
        node* last_child = first_child->prev_sibling_cyclic;
        plnnrc_assert(last_child != 0);
        last_child->next_sibling = child;
        child->prev_sibling_cyclic = last_child;
        first_child->prev_sibling_cyclic = child;
    }
    else
    {
        parent->first_child = child;
        child->prev_sibling_cyclic = child;
    }
}

void prepend_child(node* parent, node* child)
{
    plnnrc_assert(parent != 0);
    plnnrc_assert(child != 0);

    child->parent = parent;
    child->prev_sibling_cyclic = child;
    child->next_sibling = parent->first_child;

    if (parent->first_child)
    {
        parent->first_child->prev_sibling_cyclic = child;
    }

    parent->first_child = child;
}

void insert_child(node* after, node* child)
{
    plnnrc_assert(after != 0);
    plnnrc_assert(child != 0);
    plnnrc_assert(after->parent != 0);

    node* l = after;
    node* r = after->next_sibling;
    node* p = after->parent;

    l->next_sibling = child;

    if (r)
    {
        r->prev_sibling_cyclic = child;
    }
    else
    {
        p->first_child->prev_sibling_cyclic = child;
    }

    child->prev_sibling_cyclic = l;
    child->next_sibling = r;
    child->parent = p;
}

void detach_node(node* n)
{
    plnnrc_assert(n != 0);

    node* p = n->parent;
    node* n_next = n->next_sibling;
    node* n_prev = n->prev_sibling_cyclic;

    plnnrc_assert(p != 0);
    plnnrc_assert(n_prev != 0);

    if (n_next)
    {
        n_next->prev_sibling_cyclic = n_prev;
    }
    else
    {
        p->first_child->prev_sibling_cyclic = n_prev;
    }

    if (n_prev->next_sibling)
    {
        n_prev->next_sibling = n_next;
    }
    else
    {
        p->first_child = n_next;
    }

    n->parent = 0;
    n->next_sibling = 0;
    n->prev_sibling_cyclic = 0;
}

node* preorder_traversal_next(const node* root, node* current)
{
    node* n = current;

    if (n->first_child)
    {
        return n->first_child;
    }

    while (n != root && !n->next_sibling) { n = n->parent; }

    if (n == root)
    {
        return 0;
    }

    return n->next_sibling;
}

}
}
