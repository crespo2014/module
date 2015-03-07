#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <cstdio>
#include <new>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

#include <stdint.h>
#include <iostream>
#include <sys/mman.h>

#include "../../cpp-lib/posix/File.h"
#include "../queue.h"
#include "linux/types.h"

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

TEST_GROUP(QueueModule)
{
    void setup()
    {

    }
    void teardown()
    {

    }
};

std::atomic<bool> readLock;
std::atomic<unsigned> readcount { 0 };

/**
 * A thread function that read 10 times 5 bytes
 */
static void doRead(POSIX::File* f)
{
    while (readLock)
    {
    };
    char buff[5];
    unsigned i = 10;
    while (i--)
    {
        readcount += (f->read(buff, 5));

    }
}

TEST(QueueModule, fullTest)
{

    std::cout.setf(std::ios_base::unitbuf);
    try
    {
        POSIX::File f("/dev/queue", O_RDWR /*| O_NONBLOCK*/);
        queue_info_ nfo;
        nfo.block_size = 1024;
        nfo.block_count = 4;
        f.ioctl(QUEUE_INIT, &nfo);
        uint8_t* p = (uint8_t*) f.mmap(nullptr, nfo.block_count * nfo.block_size, PROT_READ | PROT_WRITE, MAP_SHARED);
        struct block_hdr_t* block[4];
        for (int i = 0; i < 4; ++i)
        {
            block[i] = reinterpret_cast<struct block_hdr_t*>(p + i * nfo.block_size);
        }
        block[0]->wr_pos_ = nfo.block_start_offset;
        block[0]->status = blck_writting;
        //char* pd = reinterpret_cast<char*>(block[0]);

        block[0]->wr_pos_ += sprintf((char*) block[0] + block[0]->wr_pos_, "1234567890123");
        // read from the file
        char b[1000];

        CHECK(f.read(b, sizeof(b), std::nothrow) == 13);

        std::thread th([&]()
        {

            std::this_thread::sleep_for(std::chrono::seconds(5));
            block[0]->wr_pos_ += sprintf((char*)block[0] + block[0]->wr_pos_,"12345");
            f.write(nullptr,0);

        });

        // blocking read to be release in 5 seconds
        CHECK(f.read(b, sizeof(b), std::nothrow) == 5);
        th.join();

        block[0]->wr_pos_ += sprintf((char*) block[0] + block[0]->wr_pos_, "1234");
        CHECK(f.read(b, sizeof(b), std::nothrow) == 4);

        //Pool for incoming data
        short events;
        //bool br;
        CHECK_FALSE(f.poll(POLLIN, events, 3000, std::nothrow));
        block[0]->wr_pos_ += sprintf((char*) block[0] + block[0]->wr_pos_, "123");
        CHECK_TRUE(f.poll(POLLIN, events, 3000, std::nothrow) == 1);
        CHECK(f.read(b, 200, std::nothrow) == 3);

        std::thread th1([&]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            block[0]->wr_pos_ += sprintf((char*)block[0] + block[0]->wr_pos_,"12345678");
            f.write(nullptr,0);
        });
        CHECK_TRUE(f.poll(POLLIN, events, 4000, std::nothrow));
        CHECK(f.read(b, 200, std::nothrow) == 8);
        th1.join();
        //jump from one buffer to another
        block[0]->status = blck_wrote;
        CHECK_FALSE(f.poll(POLLIN, events, 4000, std::nothrow));

        block[1]->wr_pos_ = nfo.block_start_offset;
        block[1]->status = blck_writting;

        //define read timeout

        // do multithread reading and check that all bytes has been read
        readLock.store(true);
        std::vector<std::thread> threads;
        threads.emplace_back(doRead, &f);        // read 10 x 5 = 50
        threads.emplace_back(doRead, &f);
        threads.emplace_back(doRead, &f);
        threads.emplace_back(doRead, &f);
        block[0]->wr_pos_ += 200;
        usleep(100);
        readLock.store(false);
        for (auto& t : threads)
        {
            t.join();
        }
        // all data has to be conssumed
        CHECK_FALSE(f.poll(POLLIN, events, 0, std::nothrow)); // to be false

        ::munmap(p, nfo.block_count * nfo.block_size);
        //s = f.read(nullptr,200,std::nothrow);

    } catch (const std::exception& e)
    {
        FAIL(e.what());
    }
}
