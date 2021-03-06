#include "persistent_memory_holder.h"
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include "../common/constants_and_types.h"
#include <iostream>
#include <utility>
#include <cstdio>
#include <cassert>
#include <unistd.h>

persistent_memory_holder::persistent_memory_holder(std::string _file_name, bool open_existing, uint64_t _size)
        : fd(-1),
          pmem_ptr(nullptr),
          file_name(std::move(_file_name)),
          size(_size)
{
    /*
     * New file should be created
     */
    if (!open_existing)
    {
        /*
         * If file already exists, delete it
         */
        remove(file_name.c_str());
        /*
         * Create new file and open it for read & write
         */
        fd = open(file_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd < 0)
        {
            throw std::runtime_error("Error while opening file " + file_name);
        }

        /*
         * Allocate enough memory in file
         */
        if (posix_fallocate(fd, 0, size) != 0)
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
    }
    else
    {
        /*
         * Open existsing file
         */
        fd = open(file_name.c_str(), O_RDWR, 0666);
        if (fd < 0)
        {
            throw std::runtime_error("Error while opening file " + file_name);
        }
    }

    /*
     * Memory-map opened file into virtual memory
     */
    void* pmemaddr = mmap(nullptr, size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

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
            std::string error_text("Error while trying to mmap file " + file_name);
            throw std::runtime_error(error_text);
        }
    }
    pmem_ptr = static_cast<uint8_t*>(pmemaddr);
    assert((uint64_t) pmemaddr % PAGE_SIZE == 0);
}

persistent_memory_holder::~persistent_memory_holder()
{
    /*
     * If object owns file, close file and unmap it
     */
    if (fd != -1 && pmem_ptr != nullptr)
    {
        if (munmap(pmem_ptr, size) == -1)
        {
            std::cerr << "Error while munmap file " << file_name << std::endl;
        }
        if (close(fd) == -1)
        {
            std::cerr << "Error while closing file " << file_name << std::endl;
        }
    }
}

const uint8_t* persistent_memory_holder::get_pmem_ptr() const
{
    return pmem_ptr;
}

uint8_t* persistent_memory_holder::get_pmem_ptr()
{
    return pmem_ptr;
}

persistent_memory_holder::persistent_memory_holder(persistent_memory_holder&& other) noexcept
        : fd(other.fd), pmem_ptr(other.pmem_ptr), file_name(std::move(other.file_name)), size(other.size)
{
    /*
     * Object, that was moved, doesn't own file anymore
     */
    other.fd = -1;
    other.pmem_ptr = nullptr;
}
