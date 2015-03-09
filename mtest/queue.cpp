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
#include "../../cpp-lib/posix/tcp_socket.h"
#include "../queue.h"
#include "linux/types.h"
#include <sys/sendfile.h>

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

char read_data[100];
char read_pos;
/**
 * read incoming data until n
 */
static void doTcpServer(tcpSock* sock)
{
    do
    {
        tcpSock client = sock->accept(std::nothrow);
        read_pos = 0;
        while (client)
        {
            read_pos += client.read(read_data + read_pos, sizeof(read_data) - read_pos);
        }
    } while (*sock);
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
        auto m = f.mmap(nullptr, nfo.block_count * nfo.block_size, PROT_READ | PROT_WRITE, MAP_SHARED);

        uint8_t* p = (uint8_t*)*m;
//        struct block_hdr_t* block[4];
//        for (int i = 0; i < 4; ++i)
//        {
//            block[i] = reinterpret_cast<struct block_hdr_t*>(p + i * nfo.block_size);
//        }
        struct block_hdr_t * pblock = reinterpret_cast<struct block_hdr_t*>(p + 0 * nfo.block_size);

        pblock->wr_pos_ = nfo.block_start_offset;
        pblock->status = blck_writting;

        pblock->wr_pos_ += sprintf((char*) pblock + pblock->wr_pos_, "1234567890123");
        // read from the file
        char b[1000];

        CHECK(f.read(b, sizeof(b), std::nothrow) == 13);

        std::thread th([&]()
        {

            std::this_thread::sleep_for(std::chrono::seconds(5));
            pblock->wr_pos_ += sprintf((char*)pblock + pblock->wr_pos_,"12345");
            f.write(nullptr,0);

        });

        // blocking read to be release in 5 seconds
        CHECK(f.read(b, sizeof(b), std::nothrow) == 5);
        th.join();

        pblock->wr_pos_ += sprintf((char*) pblock + pblock->wr_pos_, "1234");
        CHECK(f.read(b, sizeof(b), std::nothrow) == 4);

        //Pool for incoming data
        short events;
        //bool br;
        CHECK_FALSE(f.poll(POLLIN, events, 3000, std::nothrow));
        pblock->wr_pos_ += sprintf((char*) pblock + pblock->wr_pos_, "123");
        CHECK_TRUE(f.poll(POLLIN, events, 3000, std::nothrow) == 1);
        CHECK(f.read(b, 200, std::nothrow) == 3);

        std::thread th1([&]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            pblock->wr_pos_ += sprintf((char*)pblock + pblock->wr_pos_,"12345678");
            f.write(nullptr,0);
        });
        CHECK_TRUE(f.poll(POLLIN, events, 4000, std::nothrow));
        CHECK(f.read(b, 200, std::nothrow) == 8);
        th1.join();

        //jump from one buffer to another
        pblock->status = blck_wrote;
        CHECK_FALSE(f.poll(POLLIN, events, 4000, std::nothrow));

        pblock = reinterpret_cast<struct block_hdr_t*>(p + 1 * nfo.block_size);

        pblock->wr_pos_ = nfo.block_start_offset;
        pblock->status = blck_writting;

        //define read timeout

        // do multithread reading and check that all bytes has been read
        readLock.store(true);
        std::vector < std::thread > threads;
        threads.emplace_back(doRead, &f);        // read 10 x 5 = 50
        threads.emplace_back(doRead, &f);
        threads.emplace_back(doRead, &f);
        threads.emplace_back(doRead, &f);
        pblock->wr_pos_ += 200;
        usleep(100);
        readLock.store(false);
        for (auto& t : threads)
        {
            t.join();
        }
        // all data has to be conssumed
        CHECK_FALSE(f.poll(POLLIN, events, 0, std::nothrow)); // to be false

        //s = f.read(nullptr,200,std::nothrow);

    } catch (const std::exception& e)
    {
        FAIL(e.what());
    }
}

TEST(QueueModule, zero_copy)
{
    try
    {
        POSIX::File f("/dev/queue", O_RDWR /*| O_NONBLOCK*/);
        queue_info_ nfo;
        nfo.block_size = 1024;
        nfo.block_count = 4;
//        f.ioctl(QUEUE_INIT, &nfo);
//         auto m = f.mmap(nullptr, nfo.block_count * nfo.block_size, PROT_READ | PROT_WRITE, MAP_SHARED);
//        struct block_hdr_t * pblock = reinterpret_cast<struct block_hdr_t*>(p + 0 * nfo.block_size);
//
//        pblock->wr_pos_ = nfo.block_start_offset;
//        pblock->status = blck_writting;
//
//        pblock->wr_pos_ += sprintf((char*) pblock + pblock->wr_pos_, "1234567890123");
        // start a tcp server to provide data
        tcpSock sock(std::nothrow);
        CHECK_TRUE(sock);
        CHECK_TRUE(sock.setupServer(2000, std::nothrow));
        //std::thread srv(doTcpServer,&sock);

        tcpSock sock_clt("0.0.0.0", 2000/*,std::nothrow*/);
        CHECK_TRUE(sock_clt);

        //start tcp client to connect and do splice
        //pblock->wr_pos_ += 50;

        f.spliceTo(sock_clt.getfd(),50);
        f.sendFile(sock_clt.getfd(), 50);

        //CHECK(::sendfile(sock_clt.Getfd(),f.getfd(),0,50) == 50);

        //pblock->wr_pos_ += 50;
        CHECK(splice(f.getfd(), nullptr, sock_clt.getfd(), nullptr, 50, 0) == 50);
        sock_clt.shutdown(true, true);
        CHECK(read_pos == 50);
        //srv.join();

        CHECK_TRUE(sock_clt.close(std::nothrow));
        CHECK_TRUE(sock.close(std::nothrow));
        //::munmap(p, nfo.block_count * nfo.block_size);
    } catch (const std::exception& e)
    {
        FAIL(e.what());
    }
}
