//
// Created by ilya on 04.05.2020.
//

#include "gtest/gtest.h"
#include <thread>
#include "../code/persistent_stack/persistent_stack.h"
#include "../code/persistent_stack/ram_stack.h"
#include "../code/persistent_stack/call.h"
#include "test_utils.h"
#include "../code/globals/thread_local_owning_storage.h"
#include "../code/globals/thread_local_non_owning_storage.h"

TEST(persistent_stack_multithreading, add)
{
    uint32_t number_of_threads = 4;
    std::vector<std::string> file_names;
    for (uint32_t i = 0; i < number_of_threads; ++i)
    {
        const std::string file_name = get_temp_file_name("stack-" + std::to_string(i));
        file_names.push_back(file_name);
    }

    std::function<void()> normal_work = [number_of_threads, &file_names]()
    {
        std::vector<persistent_stack> stacks;
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            stacks.emplace_back(file_names[i], false);
        }
        std::vector<std::thread> threads;
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            std::function<void()> thread_action = [&stacks, i]()
            {
                uint8_t small_i = static_cast<uint8_t>(i);
                thread_local_owning_storage<ram_stack>::set_object(ram_stack());
                thread_local_non_owning_storage<persistent_stack>::ptr = &stacks[i];
                stack_frame frame_1 = stack_frame
                        {
                                "some_function_name_" + std::to_string(i),
                                std::vector<uint8_t>({1, 3, 3, 7, small_i})
                        };
                add_new_frame(
                        thread_local_owning_storage<ram_stack>::get_object(),
                        frame_1,
                        *thread_local_non_owning_storage<persistent_stack>::ptr
                );

                stack_frame frame_2 = stack_frame
                        {
                                "another_function_name_" + std::to_string(i),
                                std::vector<uint8_t>({2, 5, 1, 7, small_i})
                        };
                add_new_frame(
                        thread_local_owning_storage<ram_stack>::get_object(),
                        frame_2,
                        *thread_local_non_owning_storage<persistent_stack>::ptr
                );

                stack_frame frame_3 = stack_frame
                        {
                                "one_more_function_name_" + std::to_string(i),
                                std::vector<uint8_t>({1, 3, 5, 7, 9, small_i})
                        };
                add_new_frame(
                        thread_local_owning_storage<ram_stack>::get_object(),
                        frame_3,
                        *thread_local_non_owning_storage<persistent_stack>::ptr
                );
            };
            threads.emplace_back(std::thread(thread_action));
        }
        for (std::thread& cur_thread: threads)
        {
            cur_thread.join();
        }
    };
    normal_work();

    std::function<void()> restoration = [number_of_threads, &file_names]()
    {
        std::vector<persistent_stack> stacks;
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            stacks.emplace_back(file_names[i], true);
        }
        std::vector<std::thread> threads;
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            std::function<void()> thread_action = [&stacks, i]()
            {
                uint8_t small_i = static_cast<uint8_t>(i);
                thread_local_non_owning_storage<persistent_stack>::ptr =
                        &stacks[i];
                thread_local_owning_storage<ram_stack>::set_object(
                        read_stack(*thread_local_non_owning_storage<persistent_stack>::ptr)
                );
                ram_stack& r_stack = thread_local_owning_storage<ram_stack>::get_object();
                EXPECT_EQ(r_stack.size(), 3);

                stack_frame frame_3 = r_stack.top().frame;
                r_stack.pop();
                EXPECT_EQ(
                        frame_3.function_name,
                        "one_more_function_name_" + std::to_string(i)
                );
                EXPECT_EQ(
                        frame_3.args,
                        std::vector<uint8_t>({1, 3, 5, 7, 9, small_i})
                );

                stack_frame frame_2 = r_stack.top().frame;
                r_stack.pop();
                EXPECT_EQ(
                        frame_2.function_name,
                        "another_function_name_" + std::to_string(i)
                );
                EXPECT_EQ(
                        frame_2.args,
                        std::vector<uint8_t>({2, 5, 1, 7, small_i})
                );

                stack_frame frame_1 = r_stack.top().frame;
                r_stack.pop();
                EXPECT_EQ(
                        frame_1.function_name,
                        "some_function_name_" + std::to_string(i)
                );
                EXPECT_EQ(
                        frame_1.args,
                        std::vector<uint8_t>({1, 3, 3, 7, small_i})
                );
            };
            threads.emplace_back(std::thread(thread_action));
        }
        for (std::thread& cur_thread: threads)
        {
            cur_thread.join();
        }
    };
    restoration();
    for (std::string const& cur_file_name: file_names)
    {
        remove(cur_file_name.c_str());
    }
}

TEST(persistent_stack_multithreading, add_and_remove)
{
    uint32_t number_of_threads = 4;
    std::vector<std::string> file_names;
    for (uint32_t i = 0; i < number_of_threads; ++i)
    {
        const std::string file_name = get_temp_file_name("stack-" + std::to_string(i));
        file_names.push_back(file_name);
    }

    std::function<void()> normal_work = [number_of_threads, &file_names]()
    {
        std::vector<persistent_stack> stacks;
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            stacks.emplace_back(file_names[i], false);
        }
        std::vector<std::thread> threads;
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            std::function<void()> thread_action = [&stacks, i]()
            {
                uint8_t small_i = static_cast<uint8_t>(i);
                thread_local_owning_storage<ram_stack>::set_object(ram_stack());
                thread_local_non_owning_storage<persistent_stack>::ptr = &stacks[i];
                stack_frame frame_1 = stack_frame
                        {
                                "some_function_name_" + std::to_string(i),
                                std::vector<uint8_t>({1, 3, 3, 7, small_i})
                        };
                add_new_frame(
                        thread_local_owning_storage<ram_stack>::get_object(),
                        frame_1,
                        *thread_local_non_owning_storage<persistent_stack>::ptr
                );

                stack_frame frame_2 = stack_frame
                        {
                                "another_function_name_" + std::to_string(i),
                                std::vector<uint8_t>({2, 5, 1, 7, small_i})
                        };
                add_new_frame(
                        thread_local_owning_storage<ram_stack>::get_object(),
                        frame_2,
                        *thread_local_non_owning_storage<persistent_stack>::ptr
                );

                stack_frame frame_3 = stack_frame
                        {
                                "one_more_function_name_" + std::to_string(i),
                                std::vector<uint8_t>({1, 3, 5, 7, 9, small_i})
                        };
                add_new_frame(
                        thread_local_owning_storage<ram_stack>::get_object(),
                        frame_3,
                        *thread_local_non_owning_storage<persistent_stack>::ptr
                );
                remove_frame(
                        thread_local_owning_storage<ram_stack>::get_object(),
                        *thread_local_non_owning_storage<persistent_stack>::ptr
                );
            };
            threads.emplace_back(std::thread(thread_action));
        }
        for (std::thread& cur_thread: threads)
        {
            cur_thread.join();
        }
    };
    normal_work();

    std::function<void()> restoration = [number_of_threads, &file_names]()
    {
        std::vector<persistent_stack> stacks;
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            stacks.emplace_back(file_names[i], true);
        }
        std::vector<std::thread> threads;
        std::vector<bool> t_correct({false, false, false, false});
        for (uint32_t i = 0; i < number_of_threads; ++i)
        {
            std::function<void()> thread_action = [&stacks, i, &t_correct]()
            {
                t_correct[i] = true;
                uint8_t small_i = static_cast<uint8_t>(i);
                thread_local_non_owning_storage<persistent_stack>::ptr =
                        &stacks[i];
                thread_local_owning_storage<ram_stack>::set_object(
                        read_stack(*thread_local_non_owning_storage<persistent_stack>::ptr)
                );
                ram_stack& r_stack = thread_local_owning_storage<ram_stack>::get_object();
                t_correct[i] = t_correct[i] && (r_stack.size() == 2);

                stack_frame frame_2 = r_stack.top().frame;
                r_stack.pop();
                t_correct[i] = t_correct[i] && (frame_2.function_name ==
                                                "another_function_name_" + std::to_string(i));
                t_correct[i] = t_correct[i] &&
                               (frame_2.args == std::vector<uint8_t>({2, 5, 1, 7, small_i}));

                stack_frame frame_1 = r_stack.top().frame;
                r_stack.pop();
                t_correct[i] = t_correct[i] && (frame_1.function_name ==
                                                "some_function_name_" + std::to_string(i));
                t_correct[i] = t_correct[i] &&
                               (frame_1.args == std::vector<uint8_t>({1, 3, 3, 7, small_i}));
            };
            threads.emplace_back(std::thread(thread_action));
        }
        for (std::thread& cur_thread: threads)
        {
            cur_thread.join();
        }
        for (uint32_t i = 0; i < number_of_threads; i++)
        {
            EXPECT_TRUE(t_correct[i]);
        }
    };
    restoration();
    for (std::string const& cur_file_name: file_names)
    {
        remove(cur_file_name.c_str());
    }
}