#include <stdio.h>
#include <string.h>
#include <derplanner/runtime/runtime.h>
#include <derplanner/runtime/interface.h>
#include "travel.h"

using namespace plnnr;
using namespace travel;

struct atom_printf
{
    atom_printf()
        : current_element(0)
        , total_elements(0)
    {
    }

    void atom_begin(int atom_type, const char* name, int element_count)
    {
        total_elements = element_count;
        printf("(");
    }

    void atom_element(const int& element)
    {
        if (++current_element < total_elements)
        {
            printf("%d, ", element);
        }
        else
        {
            printf("%d", element);
        }
    }

    void atom_end(int atom_type, const char* name, int element_count)
    {
        printf(")");
    }

    int current_element;
    int total_elements;
};

struct world_printf
{
    template <typename T>
    void atom_list(int atom_type, const char* name, T* head)
    {
        printf("(%s ", name);

        for (T* tuple = head; tuple != 0; tuple = tuple->next)
        {
            atom_printf atom_visitor;
            plnnr::reflect(*tuple, atom_visitor);

            if (tuple->next)
            {
                printf(" ");
            }
        }

        printf(")\n");
    }
};

int main()
{
    const size_t tuple_list_page = 1024;

    travel::worldstate world_struct;
    memset(&world_struct, 0, sizeof(world_struct));

    world_struct.atoms[atom_start] = tuple_list::create<start_tuple>(tuple_list_page);
    world_struct.atoms[atom_finish] = tuple_list::create<finish_tuple>(tuple_list_page);
    world_struct.atoms[atom_short_distance] = tuple_list::create<short_distance_tuple>(tuple_list_page);
    world_struct.atoms[atom_long_distance] = tuple_list::create<long_distance_tuple>(tuple_list_page);
    world_struct.atoms[atom_airport] = tuple_list::create<airport_tuple>(tuple_list_page);

    plnnr::worldstate world(&world_struct);

    const int spb = 0;
    const int led = 1;
    const int svo = 2;
    const int msc = 3;

    world.append(atom<start_tuple>(spb));
    world.append(atom<finish_tuple>(msc));

    world.append(atom<short_distance_tuple>(spb, led));
    world.append(atom<short_distance_tuple>(led, spb));
    world.append(atom<short_distance_tuple>(msc, svo));
    world.append(atom<short_distance_tuple>(svo, msc));

    world.append(atom<long_distance_tuple>(spb, msc));
    world.append(atom<long_distance_tuple>(msc, spb));
    world.append(atom<long_distance_tuple>(led, svo));
    world.append(atom<long_distance_tuple>(svo, led));
    world.append(atom<long_distance_tuple>(spb, svo));
    world.append(atom<long_distance_tuple>(svo, spb));
    world.append(atom<long_distance_tuple>(msc, led));
    world.append(atom<long_distance_tuple>(led, msc));

    world.append(atom<airport_tuple>(spb, led));
    world.append(atom<airport_tuple>(msc, svo));

    world_printf printer;
    plnnr::reflect(world_struct, printer);

    plnnr::stack mstack(32768);
    plnnr::stack tstack(32768);
    plnnr::stack jstack(32768);

    planner_state pstate;
    pstate.top_method = 0;
    pstate.top_task = 0;
    pstate.mstack = &mstack;
    pstate.tstack = &tstack;
    pstate.journal = &jstack;

    bool result = find_plan(pstate, root_branch_0_expand, world.data());

    if (result)
    {
        printf("plan found:\n");

        task_instance* task = reverse_task_list(pstate.top_task);

        for (task_instance* t = task; t != 0; t = t->link)
        {
            printf("task_type=%s", task_name((task_type)t->type));

            switch (t->type)
            {
            case task_ride_taxi:
                {
                    ride_taxi_args* args = static_cast<ride_taxi_args*>(t->args);
                    printf("\t(%d, %d)\n", args->_0, args->_1);
                }
                break;
            case task_fly:
                {
                    fly_args* args = static_cast<fly_args*>(t->args);
                    printf("\t(%d, %d)\n", args->_0, args->_1);
                }
                break;
            }
        }
    }
    else
    {
        printf("plan not found.\n");
    }

    return 0;
}
