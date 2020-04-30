//
// Created by ilya on 29.04.2020.
//

#include "persistent_stack.h"
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/mman.h>
#include "../common/constants_and_types.h"
#include <iostream>
#include <utility>
#include <cstdio>


persistent_stack::persistent_stack(std::string stack_file_name, bool open_existing)
        : fd(-1),
          stack_ptr(nullptr),
          file_name(std::move(stack_file_name))
{
    if (!open_existing)
    {
        remove(file_name.c_str());
    }
    std::cout << "Opening file " << file_name << std::endl;
    if ((fd = open(file_name.c_str(), O_CREAT | O_RDWR, 0666)) < 0)
    {
        throw std::runtime_error("Error while opening file " + file_name);
    }

    std::cout << "fd = " << fd << std::endl;

    if (posix_fallocate(fd, 0, PMEM_STACK_SIZE) != 0)
    {
        if (close(fd) == -1)
        {
            std::string error_text(
                    "Error while closing file "
                    "while trying to allocate memory in file " + file_name);
            throw std::runtime_error(error_text);
        }
        else
        {
            std::string error_text(
                    "Error while trying to allocate memory in file " + file_name);
            throw std::runtime_error(error_text);
        }
    }

    void* pmemaddr =
            mmap(nullptr, PMEM_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    std::cout << "pmemaddr = " << (long long) pmemaddr << std::endl;

    if (pmemaddr == nullptr)
    {
        if (close(fd) == -1)
        {
            std::string error_text(
                    "Error while closing file "
                    "while trying to mmap file " + file_name);
            throw std::runtime_error(error_text);
        }
        else
        {
            std::string error_text(
                    "Error while trying to mmap file " + file_name);
            throw std::runtime_error(error_text);
        }
    }
    stack_ptr = static_cast<uint8_t*>(pmemaddr);
}

persistent_stack::~persistent_stack()
{
    if (fd != -1 && stack_ptr != nullptr)
    {
        std::cout << "Closing file " << file_name << ", fd = "
                  << fd << ", addr = " << (long long) stack_ptr << std::endl;
        if (munmap(stack_ptr, PMEM_STACK_SIZE) == -1)
        {
            std::cerr << "Error while munmap file " << file_name << std::endl;
        }
        if (close(fd) == -1)
        {
            std::cerr << "Error while closing file " << file_name << std::endl;
        }
    }
}

const uint8_t* persistent_stack::get_stack_ptr() const
{
    return stack_ptr;
}

uint8_t* persistent_stack::get_stack_ptr()
{
    return stack_ptr;
}

persistent_stack::persistent_stack(persistent_stack&& other) noexcept
        : fd(other.fd), stack_ptr(other.stack_ptr), file_name(std::move(other.file_name))
{
    other.fd = -1;
    other.stack_ptr = nullptr;
}
