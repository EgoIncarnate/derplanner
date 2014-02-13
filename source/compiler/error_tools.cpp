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

#include "derplanner/compiler/assert.h"
#include "derplanner/compiler/ast.h"
#include "derplanner/compiler/errors.h"
#include "formatter.h"
#include "tree_tools.h"
#include "error_tools.h"

namespace plnnrc {

void error_annotation_builder::add_argument(sexpr::node* arg)
{
    if (_node)
    {
        ast::error_ann* ann = ast::annotation<ast::error_ann>(_node);
        plnnrc_assert(ann->argument_count < ast::max_error_args);
        ann->argument_type[ann->argument_count] = ast::error_argument_node_token;
        ann->argument_node[ann->argument_count] = arg;
        ann->argument_count++;
    }
}

void error_annotation_builder::add_argument(const location& arg)
{
    if (_node)
    {
        ast::error_ann* ann = ast::annotation<ast::error_ann>(_node);
        plnnrc_assert(ann->argument_count < ast::max_error_args);
        ann->argument_type[ann->argument_count] = ast::error_argument_node_location;
        ann->argument_location[ann->argument_count++] = arg;
        ann->argument_count++;
    }
}

void error_annotation_builder::add_argument(const char* arg)
{
    if (_node)
    {
        ast::error_ann* ann = ast::annotation<ast::error_ann>(_node);
        plnnrc_assert(ann->argument_count < ast::max_error_args);
        ann->argument_type[ann->argument_count] = ast::error_argument_node_string;
        ann->argument_string[ann->argument_count++] = arg;
        ann->argument_count++;
    }
}

void error_annotation_builder::add_argument(int arg)
{
    if (_node)
    {
        ast::error_ann* ann = ast::annotation<ast::error_ann>(_node);
        plnnrc_assert(ann->argument_count < ast::max_error_args);
        ann->argument_type[ann->argument_count] = ast::error_argument_selection;
        ann->argument_selection[ann->argument_count++] = arg;
        ann->argument_count++;
    }
}

error_annotation_builder emit_error(ast::tree& ast, compilation_error error_id, sexpr::node* s_expr, bool past_token_locaion)
{
    return emit_error(ast, 0, error_id, s_expr, past_token_locaion);
}

error_annotation_builder emit_error(ast::tree& ast, ast::node* parent, compilation_error error_id, sexpr::node* s_expr, bool past_token_locaion)
{
    ast::node* error = ast.make_node(ast::node_error, s_expr);

    if (error)
    {
        if (parent)
        {
            append_child(parent, error);
        }

        if (ast.error_node_cache.size() < ast.error_node_cache.capacity())
        {
            ast.error_node_cache.append(error);
        }

        ast::error_ann* annotation = ast::annotation<ast::error_ann>(error);
        annotation->id = error_id;
        annotation->line = s_expr->line;

        if (past_token_locaion)
        {
            annotation->column = s_expr->column + static_cast<int>(strlen(s_expr->token));
        }
        else
        {
            annotation->column = s_expr->column;
        }
    }

    return error_annotation_builder(error);
}

error_annotation_builder expect_type(ast::tree& ast, sexpr::node* s_expr, sexpr::node_type expected, ast::node* parent)
{
    if (s_expr->type != expected)
    {
        return emit_error(ast, parent, error_expected, s_expr);
    }

    return 0;
}

error_annotation_builder expect_child_type(ast::tree& ast, sexpr::node* s_expr, sexpr::node_type expected, ast::node* parent)
{
    if (!s_expr->first_child)
    {
        return emit_error(ast, parent, error_expected, s_expr);
    }

    if (s_expr->first_child->type != expected)
    {
        return emit_error(ast, parent, error_expected, s_expr);
    }

    return 0;
}

error_annotation_builder expect_next_type(ast::tree& ast, sexpr::node* s_expr, sexpr::node_type expected, ast::node* parent)
{
    if (!s_expr->next_sibling)
    {
        return emit_error(ast, parent, error_expected, s_expr, true);
    }

    if (s_expr->next_sibling->type != expected)
    {
        return emit_error(ast, parent, error_expected, s_expr->next_sibling);
    }

    return 0;
}

error_annotation_builder expect_next_token(ast::tree& ast, sexpr::node* s_expr, str_ref expected, ast::node* parent)
{
    if (!s_expr->next_sibling)
    {
        return emit_error(ast, parent, error_expected, s_expr, true);
    }

    if (!is_token(s_expr->next_sibling, expected))
    {
        return emit_error(ast, parent, error_expected, s_expr->next_sibling);
    }

    return 0;
}

error_annotation_builder expect_valid_id(ast::tree& ast, sexpr::node* s_expr, ast::node* parent)
{
    plnnrc_assert(s_expr);

    if (!is_valid_id(s_expr->token))
    {
        return emit_error(ast, parent, error_invalid_id, s_expr) << s_expr;
    }

    return 0;
}

error_annotation_builder expect_condition(ast::tree& ast, sexpr::node* s_expr, bool condition, compilation_error error_id, ast::node* parent)
{
    if (!condition)
    {
        return emit_error(ast, parent, error_id, s_expr);
    }

    return 0;
}

error_annotation_builder replace_with_error(ast::tree& ast, ast::node* node, compilation_error error_id)
{
    ast::node* error_node = emit_error(ast, error_id, node->s_expr);

    node->type = error_node->type;
    node->annotation = error_node->annotation;

    return error_annotation_builder(node);
}

error_annotation_builder replace_with_error_if(bool condition, ast::tree& ast, ast::node* node, compilation_error error_id)
{
    if (condition)
    {
        return replace_with_error(ast, node, error_id);
    }

    return 0;
}

}
