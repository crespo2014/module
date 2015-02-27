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
#include "queue_mod.h"
#include "linux/types.h"

int main()
{
    POSIX::File f("/dev/queue", O_RDONLY);
    queue_info_ nfo;
    nfo.block_size = 1024;
    nfo.block_count = 4;
    f.ioctl(QUEUE_INIT,&nfo);
    uint8_t* p = (uint8_t*)f.mmap(nullptr,nfo.block_count*nfo.block_size,0);
    struct block_hdr_t* block[4];
    for(int i=0;i<4;++i)
    {
        block[i] = reinterpret_cast<struct block_hdr_t*>(p+i*nfo.block_size);
    }
    block[0]->wr_pos_ = offsetof(struct block_hdr_t,align);
    block[0]->status = blck_writting;
    char* pd = reinterpret_cast<char*>(block[0]);

    block[0]->wr_pos_ += sprintf((char*)block[0] + block[0]->wr_pos_,"user level app written data");
    // read from the file
    auto s = f.read(nullptr,200,std::nothrow);


    ::munmap(p,nfo.block_count*nfo.block_size);
    auto s2 = f.read(nullptr,200,std::nothrow);
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
