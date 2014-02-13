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

#include <unittestpp.h>
#include <derplanner/compiler/io.h>
#include "compiler/formatter.h"

namespace
{
    class buffer_writer : public plnnrc::writer
    {
    public:
        buffer_writer(size_t size)
            : buffer(0)
            , top(0)
        {
            buffer = new char[size];

            for (unsigned i = 0; i < size; ++i)
            {
                buffer[i] = 0;
            }
        }

        ~buffer_writer()
        {
            delete [] buffer;
        }

        virtual size_t write(const void* data, size_t size)
        {
            for (unsigned i = 0; i < size; ++i)
            {
                buffer[top + i] = static_cast<const char*>(data)[i];
            }

            top += size;
            return size;
        }

        virtual bool error()
        {
            return false;
        }

        char* buffer;
        unsigned top;
    };

    TEST(format_integer)
    {
        buffer_writer writer(1024);
        plnnrc::formatter formatter(writer);
        formatter.init(1);
        formatter.writeln("%d", 16384);
        formatter.flush();
        CHECK_EQUAL("16384\n", writer.buffer);
    }

    TEST(format_string_and_number)
    {
        buffer_writer writer(1024);
        plnnrc::formatter formatter(writer);
        formatter.init(1);
        formatter.writeln("%s+%d", "hello", 128);
        formatter.flush();
        CHECK_EQUAL("hello+128\n", writer.buffer);
    }

    TEST(symbol_to_id_conversion)
    {
        {
            CHECK(!plnnrc::is_valid_id("!?"));
            CHECK(plnnrc::is_valid_id("!?a"));
        }

        {
            buffer_writer writer(1024);
            plnnrc::formatter formatter(writer);
            formatter.init(1);
            formatter.writeln("%i", "abcd0123");
            formatter.flush();
            CHECK_EQUAL("abcd0123\n", writer.buffer);
        }

        {
            buffer_writer writer(1024);
            plnnrc::formatter formatter(writer);
            formatter.init(1);
            formatter.writeln("%i", "!ax-by?");
            formatter.flush();
            CHECK_EQUAL("ax_by\n", writer.buffer);
        }

        {
            buffer_writer writer(1024);
            plnnrc::formatter formatter(writer);
            formatter.init(1);
            formatter.writeln("%i", "!?23a!?");
            formatter.flush();
            CHECK_EQUAL("_23a\n", writer.buffer);
        }
    }

    TEST(paste_functor)
    {
        class paste_hello : public plnnrc::paste_func
        {
        public:
            virtual void operator()(plnnrc::formatter& output)
            {
                output.put_str("hello");
            }
        };

        buffer_writer writer(1024);
        plnnrc::formatter formatter(writer);
        formatter.init(1);
        paste_hello paste;
        formatter.writeln("%p", &paste);
        formatter.flush();
        CHECK_EQUAL("hello\n", writer.buffer);
    }
}
