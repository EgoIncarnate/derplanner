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

#ifndef DERPLANNER_COMPILER_FORMATTER_H_
#define DERPLANNER_COMPILER_FORMATTER_H_

#include <stddef.h> // size_t
#include <stdarg.h> // va_list
#include "derplanner/compiler/io.h"

namespace plnnrc {

bool is_valid_id(const char* symbol);

class formatter;

struct scope
{
    scope(formatter& output, bool end_with_empty_line=true);
    ~scope();

    formatter& output;
    bool end_with_empty_line;
};

struct class_scope
{
    class_scope(formatter& output);
    ~class_scope();

    formatter& output;
};

class paste_func
{
public:
    virtual ~paste_func() {}
    virtual void operator()(formatter& output) = 0;
};

class formatter
{
public:
    formatter(writer& output, const char* tab="\t", const char* newline="\n");
    ~formatter();

    bool init(size_t buffer_size);

    // high-level interface
    void writeln(const char* format, ...);
    void newline();

    // low-level interface
    void put_char(char c);
    void put_str(const char* s);
    void put_int(int n);
    void put_id(const char* s);
    void indent();
    void put_indent();
    void dedent();

    void flush();

private:
    formatter(const formatter&);
    const formatter& operator=(const formatter&);

    void _write(const char* format, va_list args);

    writer& _output;
    char* _buffer;
    char* _buffer_top;
    char* _buffer_end;

    int _indent_level;
    const char* _tab;
    const char* _newline;
};

}

#endif
