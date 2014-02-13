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

#ifndef DERPLANNER_COMPILER_ERRORS_H_
#define DERPLANNER_COMPILER_ERRORS_H_

namespace plnnrc {

class writer;
namespace sexpr { struct node; }
namespace ast { struct node; }

enum compilation_error
{
    error_none = 0,

    #define PLNNRC_ERROR(ID, DESC) ID,
    #include "derplanner/compiler/error_tags.inl"
    #undef PLNNRC_ERROR
};

class location
{
public:
    location(int line, int column)
        : line(line)
        , column(column)
    {
    }

    location(sexpr::node* s_expr);

    location(ast::node* node);

    location(const location& other)
        : line(other.line)
        , column(other.column)
    {
    }

    location& operator=(const location& other)
    {
        line = other.line;
        column = other.column;
        return *this;
    }

    int line;
    int column;
};

namespace ast {

enum error_argument_type
{
    error_argument_none = 0,
    error_argument_node_token,
    error_argument_node_location,
    error_argument_node_string,
};

enum { max_error_args = 4 };

struct error_ann
{
    compilation_error id;
    int line;
    int column;
    int argument_count;
    error_argument_type argument_type[max_error_args];
    sexpr::node* argument_node[max_error_args];
    location argument_location[max_error_args];
    const char* argument_string[max_error_args];
};

void format_error(error_ann* annotation, writer& stream);

}
}

#endif
