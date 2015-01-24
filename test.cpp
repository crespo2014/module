#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdint.h>
#include <iostream>

#include "../cpp-lib/posix/File.h"
#include "tsc.h"

int main()
{
    uint32_t u32;
    uint64_t u64,eu64;
    char divider =1;
    POSIX::File f("/dev/tsc", O_RDONLY);
    f.ioctl(IOCTL_RESET,&divider);
    f.ioctl(IOCTL_READ_32,&u32);
    f.ioctl(IOCTL_READ_64,&u64);
    std::cout << std::endl << u64 <<std::endl;
    for (int i =1;i<5;i++)
    {
        std::cout << "1";
    }
    f.ioctl(IOCTL_READ_64,&eu64);
    std::cout << std::endl << eu64 <<std::endl;
    std::cout << eu64 -u64 << std::endl;
    return 0;
}
