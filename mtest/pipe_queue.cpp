/*
 * pipe_queue.cpp
 *
 * Test implementation of zero copy by using a pipe and a socket.
 * Data will be copy from user space to kernel only once
 * and send over socket, otherwise that has to be copy to a temporal queue and then to kernel space
 * a memcopy in user space is removed
 *
 *  Created on: 10 Mar 2015
 *      Author: lester
 */

#include <new>
#include <ostream>
#include <iostream>

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "../cpp-lib/posix/pipe.h"
#include "../cpp-lib/posix/tcp_socket.h"
#include "../cpp-lib/posix/thread.h"

TEST_GROUP(PipeQueue)
{
	void setup()
	{
		Runnable::disableSignal(SIGPIPE);
	}
	void teardown()
	{

	}
};

TEST(PipeQueue, fullTest)
{
	// Test pipe for read write
	{
		Pipe p(std::nothrow);
		CHECK_TRUE(p);
		uint8_t dt[10];
		uint8_t rdt[50];
		for (uint8_t i = 0; i < 10; ++i)
			dt[i] = i;
		CHECK(p.write(dt,10,std::nothrow) == 10);
		CHECK(p.read(rdt,10,std::nothrow) == 10);

		for (uint8_t i = 0; i < 10; ++i)
			CHECK(rdt[i] == i);
		// test pipe for complex read write
		struct iovec vect[4] = { {dt,5},{dt+4,6},{dt+6,4},{dt+7,3} };

		CHECK(p.writev(vect,4,std::nothrow) == 18);

		// try to read more
		CHECK(p.read(rdt,30,std::nothrow) == 18);

		for (uint8_t i = 0; i < 5; ++i)
			CHECK(rdt[i] == i);
		for (uint8_t i = 4; i < 10; ++i)
			CHECK(rdt[i+5-4] == i);
		for (uint8_t i = 6; i < 10; ++i)
			CHECK(rdt[i+5+6-6] == i);
		for (uint8_t i = 7; i < 10; ++i)
			CHECK(rdt[i+5+6+4-7] == i);

		CHECK_TRUE(p.shutdownRead(std::nothrow));
		CHECK_FALSE(p);
		CHECK_TRUE(p.shutdownWrite(std::nothrow));
		CHECK_FALSE(p);
	}
}


static volatile char read_data[100];
static volatile size_t read_pos = 0;
/**
 * read incoming data until n
 */
static void doTcpServer(tcpSock* sock)
{
    do
    {
        read_pos = 0;
        tcpSock client = sock->accept(std::nothrow);
        while (client)
        {
        	auto r = client.read(const_cast<char*>(read_data) + read_pos, sizeof(read_data) - read_pos,std::nothrow);
        	if (r > 0)
        	{
        		client.write(const_cast<char*>(read_data) + read_pos, r);
        		read_pos += r;
        	}
        }
    } while (*sock);
}

TEST(PipeQueue, SockTest)
{
	// Test pipe for read write
	{
		Pipe p(std::nothrow);
		CHECK_TRUE(p);
		uint8_t dt[10];
		uint8_t rdt[10];
		for (uint8_t i = 0; i < 10; ++i)
			dt[i] = i;
		CHECK(p.write(dt,10,std::nothrow) == 10);
		//CHECK(p.read(rdt,10,std::nothrow) == 10);

		tcpSock sock(std::nothrow);
		CHECK_TRUE(sock);
		CHECK_TRUE(sock.setupServer(2000, std::nothrow));
		std::thread srv(doTcpServer,&sock);

		tcpSock sock_clt("0.0.0.0", 2000,std::nothrow);
		CHECK_TRUE(sock_clt);

		CHECK(p.spliceTo(sock_clt.getfd(),10) == 10);
		CHECK(p.write(dt,10,std::nothrow) == 10);
		CHECK(p.spliceTo(sock_clt.getfd(),10) == 10);
		sleep(2);
		CHECK(read_pos == 20);
		for (uint8_t i = 0; i < 10; ++i)
		{
			CHECK(read_data[i] == i);
			CHECK(read_data[i+10] == i);
		}
		CHECK(p.spliceFrom(sock_clt.getfd(),20) == 20);

		CHECK(p.read(const_cast<char*>(read_data),20,std::nothrow) == 20);
		for (uint8_t i = 0; i < 10; ++i)
		{
			CHECK(read_data[i] == i);
			CHECK(read_data[i+10] == i);
		}
		sock_clt.shutdown(false,true);
		sock.shutdown(true,true);
		sleep(1);
		CHECK_TRUE(sock.close(std::nothrow));
		srv.join();
		CHECK_TRUE(sock_clt.close(std::nothrow));

		CHECK_TRUE(p.shutdownRead(std::nothrow));
		CHECK_FALSE(p);
		CHECK_TRUE(p.shutdownWrite(std::nothrow));
		CHECK_FALSE(p);
	}
}
// TODO - send a splice command to the queue with a fd and th queue automatcally will derive data to the fd
