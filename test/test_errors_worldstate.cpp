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
#include <unittestpp.h>
#include <derplanner/compiler/s_expression.h>
#include <derplanner/compiler/ast.h>
#include <derplanner/compiler/ast_build.h>
#include <derplanner/compiler/errors.h>

using namespace plnnrc;

namespace
{
    struct buffer_context
    {
        buffer_context(size_t bytes)
            : bytes(bytes)
        {
            data = new char[bytes];
        }

        ~buffer_context()
        {
            delete [] data;
        }

        size_t bytes;
        char* data;
    };

    void test_error(const char* code, compilation_error error_id, int line, int column)
    {
        buffer_context buffer(strlen(code) + 1);
        strncpy(buffer.data, code, buffer.bytes);
        sexpr::tree expr;
        expr.parse(buffer.data);
        ast::tree tree;
        ast::build_translation_unit(tree, expr.root());
        CHECK(tree.error_node_cache.size());
        ast::node* error_node = tree.error_node_cache[0];
        ast::error_ann* error_ann = ast::annotation<ast::error_ann>(error_node);
        CHECK_EQUAL(error_id, error_ann->id);
        CHECK_EQUAL(line, error_ann->line);
        CHECK_EQUAL(column, error_ann->column);
    }

    TEST(test_1) { test_error("(:worldstate)", error_expected_type, 1, 13); }
    TEST(test_2) { test_error("(:worldstate test)", error_expected_type, 1, 14); }
}
