#include "gtest/gtest.h"
#include "../../code/persistent_memory/persistent_memory_holder.h"
#include "../../code/persistent_stack/persistent_stack.h"
#include "../../code/storage/global_storage.h"
#include "../common/test_utils.h"
#include <functional>
#include "../../code/model/function_address_holder.h"
#include "../../code/runtime/call.h"

namespace
{
    void f(const uint8_t*)
    {
        do_call("g", std::vector<uint8_t>({4, 5, 6}));
    }

    void g(const uint8_t*)
    {
        do_call("h", std::vector<uint8_t>({7, 8, 9}));
        throw std::runtime_error("Ha-ha, crash goes brrrrrr");
    }

    void h(const uint8_t*)
    {
    }
}

TEST(call, restoration_after_crash)
{
    temp_file file(get_temp_file_name("stack"));
    std::function<void()> execution = [&file]()
    {
        global_storage<function_address_holder>::set_object(function_address_holder());
        global_storage<function_address_holder>::get_object().funcs.clear();
        global_storage<function_address_holder>::get_object().funcs["f"] = std::make_pair(f, f);
        global_storage<function_address_holder>::get_object().funcs["g"] = std::make_pair(g, g);
        global_storage<function_address_holder>::get_object().funcs["h"] = std::make_pair(h, h);
        persistent_memory_holder p_stack(file.file_name, false, PMEM_STACK_SIZE);
        thread_local_non_owning_storage<persistent_memory_holder>::ptr = &p_stack;
        thread_local_owning_storage<ram_stack>::set_object(ram_stack());
        do_call("f", std::vector<uint8_t>({1, 2, 3}));
    };
    try
    {
        execution();
    } catch (...)
    {

    }


    std::function<void()> restoration = [&file]()
    {
        persistent_memory_holder p_stack(file.file_name, true, PMEM_STACK_SIZE);
        thread_local_non_owning_storage<persistent_memory_holder>::ptr = &p_stack;
        ram_stack r_stack = read_stack(p_stack);
        thread_local_owning_storage<ram_stack>::set_object(r_stack);
        EXPECT_EQ(r_stack.size(), 2);

        stack_frame g_frame = r_stack.get_last_frame().get_frame();
        r_stack.remove_frame();
        EXPECT_EQ(g_frame.get_function_name(), "g");
        EXPECT_EQ(g_frame.get_args(), std::vector<uint8_t>({4, 5, 6}));

        stack_frame f_frame = r_stack.get_last_frame().get_frame();
        r_stack.remove_frame();
        EXPECT_EQ(f_frame.get_function_name(), "f");
        EXPECT_EQ(f_frame.get_args(), std::vector<uint8_t>({1, 2, 3}));
    };
    restoration();
}