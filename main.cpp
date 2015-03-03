#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <cstdio>
#include <new>
#include <thread>
#include <chrono>


#include <stdint.h>
#include <iostream>
#include <sys/mman.h>

#include "../cpp-lib/posix/File.h"
#include "queue_mod.h"
#include "linux/types.h"

int main()
{
    std::cout.setf(std::ios_base::unitbuf);
    try
    {
    POSIX::File f("/dev/queue", O_RDWR /*| O_NONBLOCK*/);
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
    std::cout << "1";
    auto s = f.read(b,sizeof(b),std::nothrow);
    std::cout << "1";

        std::thread th([&]()
        {
            std::cout << "2";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            block[0]->wr_pos_ += sprintf((char*)block[0] + block[0]->wr_pos_,"from thread");
            f.write(nullptr,0);
            std::cout << "2";
        });

    // blocking read to be release in 5 seconds
    s = f.read(b,sizeof(b),std::nothrow);
    th.join();
    std::cout << "1";

    block[0]->wr_pos_ += sprintf((char*)block[0] + block[0]->wr_pos_,"more data");
    s = f.read(b,sizeof(b),std::nothrow);
    std::cout << "1";

    //Pool for incoming data
    short events;
    bool br;
    br = f.poll(POLLIN,events,3000,std::nothrow);
    block[0]->wr_pos_ += sprintf((char*)block[0] + block[0]->wr_pos_,"from thread");
    br = f.poll(POLLIN,events,3000,std::nothrow);
    s = f.read(b,200,std::nothrow);
    std::cout << "1";

    std::thread th1([&]()
    {
        std::cout << "2";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        block[0]->wr_pos_ += sprintf((char*)block[0] + block[0]->wr_pos_,"from thread");
        f.write(nullptr,0);
    });
    br = f.poll(POLLIN,events,4000,std::nothrow);
    s = f.read(b,200,std::nothrow);
    std::cout << "1";
    th1.join();
    //jump from one buffer to another

    //define read timeout


    ::munmap(p,nfo.block_count*nfo.block_size);
    //s = f.read(nullptr,200,std::nothrow);


    } catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
