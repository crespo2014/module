#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdint.h>
#include <iostream>
#include <sys/mman.h>

#include "../cpp-lib/posix/File.h"
#include "tsc.h"
#include "queue_mod.h"
#include "linux/types.h"

int main()
{
    POSIX::File f("/dev/queue", O_RDONLY);
    queue_info_ nfo;
    nfo.block_size = 1024;
    nfo.block_count = 4;
    f.ioctl(QUEUE_INIT,&nfo);
    void* p = f.mmap(nullptr,nfo.block_count*nfo.block_size,0);
    ::munmap(p,nfo.block_count*nfo.block_size);
    /*
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
    */
    return 0;
}
