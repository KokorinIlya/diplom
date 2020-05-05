#include "test_utils.h"
#include <filesystem>
#include <cstdio>

std::string get_temp_file_name(std::string const& path_prefix)
{
    while (true)
    {
        int suffix = rand();
        std::string cur_name = path_prefix + "_" + std::to_string(suffix) + ".tmp";
        if (std::filesystem::exists(cur_name))
        {
            continue;
        }
        return cur_name;
    }

}

temp_file::temp_file(std::string name) : file_name(std::move(name))
{}

temp_file::~temp_file()
{
    remove(file_name.c_str());
}
