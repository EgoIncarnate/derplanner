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

#ifndef DERPLANNER_RUNTIME_INTERFACE_H_
#define DERPLANNER_RUNTIME_INTERFACE_H_

#include <derplanner/runtime/worldstate.h>

namespace plnnr {

class worldstate
{
public:
    worldstate(void* data);

    template <typename T>
    void append(const T& tuple);

    void* data() const { return _data; }

private:
    void* _data;
};

template <typename W, typename V>
struct world_reflector
{
    void operator()(const W& world, V& visitor) {}
};

template <typename W, typename V>
void reflect_world(const W& world, V& visitor)
{
    world_reflector<W, V> reflector;
    reflector(world, visitor);
}

}

#include <derplanner/runtime/interface.inl>

#endif
