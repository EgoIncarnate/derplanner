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

#ifndef DERPLANNER_COMPILER_S_EXPRESSION_H_
#define DERPLANNER_COMPILER_S_EXPRESSION_H_

namespace derplanner {
namespace s_expression {

enum node_type
{
    node_none = 0,
    node_list,
    node_symbol,
    node_int,
    node_float,
};

struct node
{
    node_type type;
    int line;
    int column;
    char* token;
    node* parent;
    node* first_child;
    node* next_sibling;
    node* prev_sibling_cyclic;
};

int as_int(const node& n);
float as_float(const node& n);

enum parse_status
{
    parse_ok = 0,
    parse_mismatch_opening,
    parse_mismatch_closing,
    parse_out_of_memory,
};

class tree
{
public:
    tree();
    ~tree();

    parse_status parse(char* buffer);
    node* root() const;

private:
    tree(const tree&);
    const tree& operator=(const tree&);

    void* _memory;
    node* _root;
};

}
}

#endif
