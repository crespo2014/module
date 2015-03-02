/*
 * tsc_test.cpp
 *
 *  Created on: 2 Mar 2015
 *      Author: lester
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <cstdio>
#include <new>

#include <stdint.h>
#include <iostream>
#include <sys/mman.h>

#include "../cpp-lib/posix/File.h"
#include "linux/types.h"
#include "tsc.h"

int main()
{
    try
    {
        uint32_t u32;
        uint64_t u64, eu64;
        char divider = 1;
        POSIX::File f("/dev/tsc", O_RDONLY);
        f.ioctl(IOCTL_RESET, &divider);
        f.ioctl(IOCTL_READ_32, &u32);
        f.ioctl(IOCTL_READ_64, &u64);
        std::cout << std::endl << u64 << std::endl;
        for (int i = 1; i < 5; i++)
        {
            std::cout << "1";
        }
        f.ioctl(IOCTL_READ_64, &eu64);
        std::cout << std::endl << eu64 << std::endl;
        std::cout << eu64 - u64 << std::endl;
    } catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
