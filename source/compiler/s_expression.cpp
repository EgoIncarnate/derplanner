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

#include <stdlib.h>
#include "derplanner/compiler/derplanner_assert.h"
#include "derplanner/compiler/derplanner_memory.h"
#include "derplanner/compiler/s_expression.h"

namespace plnnrc {
namespace sexpr {

namespace
{
    const size_t chunk_node_count = 2048;

    struct node_chunk
    {
        node_chunk* next;
        size_t top;
        node data[chunk_node_count];
    };

    void free_chunks(void* memory)
    {
        for (node_chunk* chunk = reinterpret_cast<node_chunk*>(memory); chunk != 0;)
        {
            node_chunk* next = chunk->next;
            memory::deallocate(chunk);
            chunk = next;
        }
    }

    node* alloc_node(void*& memory)
    {
        node_chunk* chunk = reinterpret_cast<node_chunk*>(memory);

        if (!chunk || chunk->top >= chunk_node_count)
        {
            node_chunk* new_chunk = reinterpret_cast<node_chunk*>(memory::allocate(sizeof(node_chunk)));

            if (!new_chunk)
            {
                return 0;
            }

            new_chunk->next = chunk;
            new_chunk->top = 0;

            chunk = new_chunk;
            memory = new_chunk;
        }

        chunk->top++;

        return chunk->data + chunk->top;
    }

    enum token_type
    {
        token_none = 0,
        token_lp,
        token_rp,
        token_symbol,
        token_int,
        token_float,
    };

    struct parse_state
    {
        void** tree_memory;
        int line;
        int column;
        char* cursor;
        char* cursor_next;
        char* null_location;
        node* parent;
    };

    void init_state(parse_state& state, char* buffer, void** memory)
    {
        state.tree_memory = memory;
        state.line = 1;
        state.column = 1;
        state.cursor = buffer;
        state.cursor_next = buffer;
        state.null_location = 0;
        state.parent = 0;
    }

    void move(parse_state& state)
    {
        state.cursor++;
        state.column++;
    }

    node* append_child(parse_state& state)
    {
        node* n = alloc_node(*state.tree_memory);
        node* p = state.parent;

        if (!n)
        {
            return 0;
        }

        n->parent = p;
        n->first_child = 0;
        n->next_sibling = 0;
        n->prev_sibling_cyclic = 0;

        if (p)
        {
            node* first_child = p->first_child;

            if (first_child)
            {
                node* last_child = first_child->prev_sibling_cyclic;
                last_child->next_sibling = n;
                n->prev_sibling_cyclic = last_child;
                first_child->prev_sibling_cyclic = n;
            }
            else
            {
                p->first_child = n;
                n->prev_sibling_cyclic = n;
            }
        }

        return n;
    }

    node* push_list(parse_state& state)
    {
        node* n = append_child(state);

        if (!n)
        {
            return 0;
        }

        n->type = node_list;
        n->line = state.line;
        n->column = state.column;
        n->token = 0;

        state.parent = n;

        return n;
    }

    node* pop_list(parse_state& state)
    {
        node* n = state.parent;

        if (n)
        {
            state.parent = n->parent;
            return n;
        }

        return 0;
    }

    node* append_node(parse_state& state, node_type type)
    {
        node* n = append_child(state);

        if (!n)
        {
            return 0;
        }

        n->type = type;
        n->line = state.line;
        n->column = state.column;
        n->token = state.cursor;
        return n;
    }

    bool buffer_left(parse_state& state)
    {
        return *state.cursor_next != '\0';
    }

    void increment_line(parse_state& state)
    {
        char c = *state.cursor;

        move(state);

        if ((*state.cursor == '\n' || *state.cursor == '\r') && (*state.cursor != c))
        {
            move(state);
        }

        state.line++;
        state.column = 1;
    }

    bool is_delimeter(parse_state& state)
    {
        switch (*state.cursor)
        {
        case '\0':
        case '\n': case '\r':
        case ' ': case '\f': case '\t': case '\v':
        case '(': case ')':
            return true;
        }

        return false;
    }

    void scan_symbol(parse_state& state)
    {
        char* begin = state.cursor;
        int line = state.line;
        int column = state.column;

        while (!is_delimeter(state))
        {
            move(state);
        }

        state.cursor_next = state.cursor;
        state.null_location = state.cursor;
        state.cursor = begin;
        state.line = line;
        state.column = column;
    }

    int scan_number_char_class(parse_state& state)
    {
        switch (*state.cursor)
        {
        case '+': case '-':
            return 0;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return 1;
        case '.':
            return 2;
        case 'e': case 'E':
            return 3;
        }

        return -1;
    }

    int scan_number_transitions[] = {
         1,  3,  2, -1,
        -1,  3,  2, -1,
        -1,  4, -1, -1,
        -1,  3,  4,  5,
        -1,  4, -1,  5,
         6,  7, -1, -1,
        -1,  7, -1, -1,
        -1,  7, -1, -1,
    };

    token_type scan_number(parse_state& state)
    {
        char* begin = state.cursor;
        int line = state.line;
        int column = state.column;

        int s = 0;

        while (!is_delimeter(state))
        {
            int c = scan_number_char_class(state);

            if (c < 0)
            {
                return token_none;
            }

            int n = scan_number_transitions[s * 4 + c];

            if (n < 0)
            {
                return token_none;
            }

            s = n;

            move(state);
        }

        state.cursor_next = state.cursor;
        state.null_location = state.cursor;
        state.cursor = begin;
        state.line = line;
        state.column = column;

        switch (s)
        {
        case 3:
            return token_int;
        case 4: case 7:
            return token_float;
        }

        return token_none;
    }

    void null_terminate(parse_state& state)
    {
        if (state.null_location)
        {
            *state.null_location = '\0';
            state.null_location = 0;
        }
    }

    token_type next_token(parse_state& state)
    {
        state.cursor = state.cursor_next;

        while (*state.cursor)
        {
            switch (*state.cursor)
            {
            case '\n': case '\r':
                increment_line(state);
                null_terminate(state);
                break;
            case ' ': case '\f': case '\t': case '\v':
                move(state);
                null_terminate(state);
                break;
            case ';':
                while (*state.cursor != '\n' && *state.cursor != '\r' && *state.cursor != '\0')
                {
                    move(state);
                }
                null_terminate(state);
                break;
            case '(':
                move(state);
                state.cursor_next = state.cursor;
                null_terminate(state);
                return token_lp;
            case ')':
                move(state);
                state.cursor_next = state.cursor;
                null_terminate(state);
                return token_rp;
            default:
                {
                    token_type t = scan_number(state);

                    if (t != token_none)
                    {
                        return t;
                    }

                    scan_symbol(state);
                    return token_symbol;
                }
            }
        }

        return token_none;
    }

} // unnamed namespace

tree::tree()
    : _memory(0)
    , _root(0)
{
}

tree::~tree()
{
    if (_memory)
    {
        free_chunks(_memory);
    }
}

parse_status tree::parse(char* buffer)
{
    plnnrc_assert(buffer != 0);

    if (_memory)
    {
        free_chunks(_memory);
        _memory = 0;
        _root = 0;
    }

    parse_state state;
    init_state(state, buffer, &_memory);

    _root = push_list(state);

    if (!_root)
    {
        return parse_out_of_memory;
    }

    token_type root_token = next_token(state);

    if (root_token != token_lp && root_token != token_none)
    {
        return parse_expected_lp;
    }

    while (buffer_left(state))
    {
        switch (next_token(state))
        {
        case token_lp:
            {
                if (!push_list(state))
                {
                    return parse_out_of_memory;
                }
            }
            break;
        case token_rp:
            {
                if (!pop_list(state))
                {
                    return parse_excess_close;
                }
            }
            break;
        case token_int:
            {
                if (!append_node(state, node_int))
                {
                    return parse_out_of_memory;
                }
            }
            break;
        case token_float:
            {
                if (!append_node(state, node_float))
                {
                    return parse_out_of_memory;
                }
            }
            break;
        case token_symbol:
            {
                if (!append_node(state, node_symbol))
                {
                    return parse_out_of_memory;
                }
            }
            break;
        default:
            plnnrc_assert(false);
            break;
        }
    }

    if (state.parent)
    {
        return parse_excess_open;
    }

    return parse_ok;
}

float as_float(const node& n)
{
    return static_cast<float>(strtod(n.token, 0));
}

int as_int(const node& n)
{
    return static_cast<int>(strtol(n.token, 0, 10));
}

}
}
