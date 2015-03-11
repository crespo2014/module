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

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "../cpp-lib/posix/pipe.h"

TEST_GROUP(PipeQueue)
{
	void setup()
	{

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

TEST(PipeQueue, tcpSock)
{
}
}
