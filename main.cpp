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
    try
    {
    POSIX::File f("/dev/queue", O_RDWR);
    queue_info_ nfo;
    nfo.block_size = 1024;
    nfo.block_count = 4;
    f.ioctl(QUEUE_INIT,&nfo);
    uint8_t* p = (uint8_t*)f.mmap(nullptr,nfo.block_count*nfo.block_size,PROT_READ | PROT_WRITE,MAP_SHARED);
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
    char b[1000];
    auto s = f.read(b,sizeof(b),std::nothrow);


    ::munmap(p,nfo.block_count*nfo.block_size);
    auto s2 = f.read(nullptr,200,std::nothrow);

    } catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
