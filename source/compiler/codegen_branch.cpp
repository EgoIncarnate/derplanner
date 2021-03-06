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
#include "derplanner/compiler/s_expression.h"
#include "derplanner/compiler/ast.h"
#include "ast_tools.h"
#include "formatter.h"
#include "codegen_branch.h"

namespace plnnrc {

class paste_function_call : public paste_func
{
public:
    ast::node* function_call;

    paste_function_call(ast::node* function_call)
        : function_call(function_call)
    {
    }

    virtual void operator()(formatter& output)
    {
        output.put_id(function_call->s_expr->token);
        output.put_char('(');

        for (ast::node* argument = function_call->first_child; argument != 0; argument = argument->next_sibling)
        {
            switch (argument->type)
            {
            case ast::node_term_variable:
                {
                    ast::node* def = definition(argument);
                    plnnrc_assert(def);
                    int var_index = ast::annotation<ast::term_ann>(def)->var_index;

                    if (is_operator_parameter(def))
                    {
                        output.put_str("a->_");
                        output.put_int(var_index);
                    }
                    else if (is_method_parameter(def))
                    {
                        output.put_str("method_args->_");
                        output.put_int(var_index);
                    }
                    else
                    {
                        output.put_str("precondition->_");
                        output.put_int(var_index);
                    }
                }
                break;
            case ast::node_term_call:
                {
                    paste_function_call paste(argument);
                    output.put_str("wstate->");
                    paste(output);
                }
                break;
            default:
                // unsupported argument type
                plnnrc_assert(false);
            }

            if (!is_last(argument))
            {
                output.put_str(", ");
            }
        }

        output.put_char(')');
    }
};

void generate_branch_expands(ast::tree& ast, ast::node* domain, formatter& output)
{
    unsigned precondition_index = 0;

    for (ast::node* method = domain->first_child; method != 0; method = method->next_sibling)
    {
        if (!ast::is_method(method))
        {
            continue;
        }

        ast::node* atom = method->first_child;
        const char* method_name = atom->s_expr->token;

        unsigned branch_index = 0;

        for (ast::node* branch = method->first_child->next_sibling; branch != 0; branch = branch->next_sibling)
        {
            plnnrc_assert(ast::is_branch(branch));

            ast::branch_ann* ann = ast::annotation<ast::branch_ann>(branch);

            ast::node* precondition = branch->first_child;
            ast::node* tasklist = precondition->next_sibling;

            plnnrc_assert(ast::is_task_list(tasklist));

            output.writeln("bool %i_branch_%d_expand(method_instance* method, planner_state& pstate, void* world)", method_name, branch_index);
            {
                scope s(output);

                output.writeln("p%d_state* precondition = plnnr::precondition<p%d_state>(method);", precondition_index, precondition_index);

                if (has_parameters(method))
                {
                    output.writeln("%i_args* method_args = plnnr::arguments<%i_args>(method);", method_name, method_name);
                }

                output.writeln("worldstate* wstate = static_cast<worldstate*>(world);");

                output.newline();
                output.writeln("PLNNR_COROUTINE_BEGIN(*method);");
                output.newline();

                output.writeln("precondition = push_precondition<p%d_state>(pstate, method);", precondition_index);

                for (ast::node* param = atom->first_child; param != 0; param = param->next_sibling)
                {
                    ast::node* var = first_parameter_usage(param, precondition);

                    if (var)
                    {
                        int param_index = ast::annotation<ast::term_ann>(param)->var_index;
                        int var_index = ast::annotation<ast::term_ann>(var)->var_index;

                        output.writeln("precondition->_%d = method_args->_%d;", var_index, param_index);
                    }
                }

                output.newline();

                output.writeln("while (next(*precondition, *wstate))");
                {
                    scope s(output);

                    if (!tasklist->first_child)
                    {
                        if (!ann->foreach)
                        {
                            output.writeln("method->flags |= method_flags_expanded;");
                        }

                        output.writeln("PLNNR_COROUTINE_YIELD(*method);");
                    }

                    for (ast::node* task_atom = tasklist->first_child; task_atom != 0; task_atom = task_atom->next_sibling)
                    {
                        {
                            scope s(output);

                            if (ast::is_add_list(task_atom))
                            {
                                generate_effects_add(task_atom, output);
                            }
                            else if (ast::is_delete_list(task_atom))
                            {
                                generate_effects_delete(task_atom, output);
                            }
                            else if (is_lazy(task_atom))
                            {
                                generate_operator_task(ast, method, task_atom, output);
                            }
                            else if (is_operator(ast, task_atom))
                            {
                                generate_operator_task(ast, method, task_atom, output);
                            }
                            else if (is_method(ast, task_atom))
                            {
                                generate_method_task(ast, method, task_atom, output);
                            }
                            else
                            {
                                // unknown construct in task list
                                plnnrc_assert(false);
                            }
                        }

                        if (is_last(task_atom) && !ann->foreach)
                        {
                            output.writeln("method->flags |= method_flags_expanded;");
                        }

                        if (is_last(task_atom) || !is_effect_list(task_atom))
                        {
                            output.writeln("PLNNR_COROUTINE_YIELD(*method);");

                            if (!is_last(task_atom))
                            {
                                output.newline();

                                if (is_method(ast, task_atom))
                                {
                                    output.writeln("if (method->flags & method_flags_failed)");
                                    {
                                        scope s(output, true);
                                        output.writeln("continue;");
                                    }
                                }
                            }
                        }
                    }
                }

                if (ann->foreach)
                {
                    output.writeln("if (precondition->stage > 0)");
                    {
                        scope s(output);
                        output.writeln("method->flags |= method_flags_expanded;");
                        output.writeln("PLNNR_COROUTINE_YIELD(*method);");
                    }
                }

                if (!is_last(branch))
                {
                    output.writeln("return expand_next_branch(pstate, %i_branch_%d_expand, world);", method_name, branch_index+1);
                }

                output.writeln("PLNNR_COROUTINE_END();");

                ++branch_index;
                ++precondition_index;
            }
        }
    }
}

void generate_operator_effects(ast::tree& ast, ast::node* /*method*/, ast::node* task_atom, formatter& output)
{
    ast::node* operatr = ast.operators.find(task_atom->s_expr->token);
    plnnrc_assert(operatr);

    ast::node* effects_delete = operatr->first_child->next_sibling;
    ast::node* effects_add = effects_delete->next_sibling;
    plnnrc_assert(effects_delete && effects_add);

    if (effects_delete->first_child)
    {
        output.newline();

        generate_effects_delete(effects_delete, output);

        if (effects_add->first_child)
        {
            output.newline();
        }
    }

    if (effects_add->first_child)
    {
        if (!effects_delete->first_child)
        {
            output.newline();
        }

        generate_effects_add(effects_add, output);
    }
}

void generate_effects_add(ast::node* effects, formatter& output)
{
    for (ast::node* effect = effects->first_child; effect != 0; effect = effect->next_sibling)
    {
        scope s(output, !is_last(effect));

        const char* atom_id = effect->s_expr->token;

        output.writeln("tuple_list::handle* list = wstate->atoms[atom_%i];", atom_id, atom_id);
        output.writeln("%i_tuple* tuple = tuple_list::append<%i_tuple>(list);", atom_id, atom_id);

        int param_index = 0;

        for (ast::node* arg = effect->first_child; arg != 0; arg = arg->next_sibling)
        {
            if (ast::is_term_variable(arg))
            {
                ast::node* def = definition(arg);
                plnnrc_assert(def);
                int var_index = ast::annotation<ast::term_ann>(def)->var_index;

                if (is_operator_parameter(def))
                {
                    output.writeln("tuple->_%d = a->_%d;", param_index, var_index);
                }
                else if (is_method_parameter(def))
                {
                    output.writeln("tuple->_%d = method_args->_%d;", param_index, var_index);
                }
                else
                {
                    output.writeln("tuple->_%d = precondition->_%d;", param_index, var_index);
                }
            }

            if (ast::is_term_call(arg))
            {
                paste_function_call paste(arg);
                output.writeln("tuple->_%d = wstate->%p;", param_index, &paste);
            }

            ++param_index;
        }

        output.writeln("operator_effect* effect = push<operator_effect>(pstate.journal);");
        output.writeln("effect->tuple = tuple;");
        output.writeln("effect->list = list;");
    }
}

void generate_effects_delete(ast::node* effects, formatter& output)
{
    for (ast::node* effect = effects->first_child; effect != 0; effect = effect->next_sibling)
    {
        const char* atom_id = effect->s_expr->token;

        output.writeln("for (%i_tuple* tuple = tuple_list::head<%i_tuple>(wstate->atoms[atom_%i]); tuple != 0; tuple = tuple->next)", atom_id, atom_id, atom_id);
        {
            scope s(output, !is_last(effect));

            int param_index = 0;

            for (ast::node* arg = effect->first_child; arg != 0; arg = arg->next_sibling)
            {
                if (ast::is_term_variable(arg))
                {
                    ast::node* def = definition(arg);
                    plnnrc_assert(def);
                    int var_index = ast::annotation<ast::term_ann>(def)->var_index;

                    if (is_operator_parameter(def))
                    {
                        output.writeln("if (tuple->_%d != a->_%d)", param_index, var_index);
                    }
                    else if (is_method_parameter(def))
                    {
                        output.writeln("if (tuple->_%d != method_args->_%d)", param_index, var_index);
                    }
                    else
                    {
                        output.writeln("if (tuple->_%d != precondition->_%d)", param_index, var_index);
                    }
                }

                if (ast::is_term_call(arg))
                {
                    paste_function_call paste(arg);
                    output.writeln("if (tuple->_%d != wstate->%p)", param_index, &paste);
                }

                {
                    scope s(output);
                    output.writeln("continue;");
                }

                ++param_index;
            }

            output.writeln("tuple_list::handle* list = wstate->atoms[atom_%i];", atom_id, atom_id);
            output.writeln("operator_effect* effect = push<operator_effect>(pstate.journal);");
            output.writeln("effect->tuple = tuple;");
            output.writeln("effect->list = list;");
            output.writeln("tuple_list::detach(list, tuple);");
            output.newline();
            output.writeln("break;");
        }
    }
}

void generate_operator_task(ast::tree& ast, ast::node* method, ast::node* task_atom, formatter& output)
{
    plnnrc_assert(is_lazy(task_atom) || is_operator(ast, task_atom));

    if (is_lazy(task_atom))
    {
        output.writeln("task_instance* t = push_task(pstate, task_%i, %i_branch_0_expand);", task_atom->s_expr->token, task_atom->s_expr->token);
    }
    else
    {
        output.writeln("task_instance* t = push_task(pstate, task_%i, 0);", task_atom->s_expr->token);
    }

    if (task_atom->first_child)
    {
        output.writeln("%i_args* a = push_arguments<%i_args>(pstate, t);", task_atom->s_expr->token, task_atom->s_expr->token);
    }

    int param_index = 0;

    for (ast::node* arg = task_atom->first_child; arg != 0; arg = arg->next_sibling)
    {
        if (ast::is_term_variable(arg))
        {
            ast::node* def = definition(arg);
            plnnrc_assert(def);
            int var_index = ast::annotation<ast::term_ann>(def)->var_index;

            if (is_parameter(def))
            {
                output.writeln("a->_%d = method_args->_%d;", param_index, var_index);
            }
            else
            {
                output.writeln("a->_%d = precondition->_%d;", param_index, var_index);
            }
        }

        if (ast::is_term_call(arg))
        {
            paste_function_call paste(arg);
            output.writeln("a->_%d = wstate->%p;", param_index, &paste);
        }

        ++param_index;
    }

    if (!is_lazy(task_atom))
    {
        generate_operator_effects(ast, method, task_atom, output);
    }
}

void generate_method_task(ast::tree& ast, ast::node* /*method*/, ast::node* task_atom, formatter& output)
{
    plnnrc_assert(is_method(ast, task_atom));
    (void)(ast);

    output.writeln("method_instance* t = push_method(pstate, task_%i, %i_branch_0_expand);", task_atom->s_expr->token, task_atom->s_expr->token);

    if (task_atom->first_child)
    {
        output.writeln("%i_args* a = push_arguments<%i_args>(pstate, t);", task_atom->s_expr->token, task_atom->s_expr->token);
    }

    int param_index = 0;

    for (ast::node* arg = task_atom->first_child; arg != 0; arg = arg->next_sibling)
    {
        if (ast::is_term_variable(arg))
        {
            ast::node* def = definition(arg);
            plnnrc_assert(def);
            int var_index = ast::annotation<ast::term_ann>(def)->var_index;

            if (is_parameter(def))
            {
                output.writeln("a->_%d = method_args->_%d;", param_index, var_index);
            }
            else
            {
                output.writeln("a->_%d = precondition->_%d;", param_index, var_index);
            }
        }

        if (ast::is_term_call(arg))
        {
            paste_function_call paste(arg);
            output.writeln("a->_%d = wstate->%p;", param_index, &paste);
        }

        ++param_index;
    }
}

}
