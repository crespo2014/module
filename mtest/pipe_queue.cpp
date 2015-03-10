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
		uint8_t rdt[10];
		for (uint8_t i = 0; i < 10; ++i)
			dt[i] = i;
		CHECK(p.write(dt,10,std::nothrow) == 10);
		CHECK(p.read(rdt,10,std::nothrow) == 10);

		for (uint8_t i = 0; i < 10; ++i)
			CHECK(rdt[i] == i);
		// test pipe for complex read write


		CHECK_TRUE(p.shutdownRead(std::nothrow));
		CHECK_FALSE(p);
		CHECK_TRUE(p.shutdownWrite(std::nothrow));
		CHECK_FALSE(p);
	}


//	char *str0 = "hello ";
//	char *str1 = "world\n";
//	struct iovec iov[2];
//	ssize_t nwritten;
//
//	iov[0].iov_base = str0;
//	iov[0].iov_len = strlen(str0);
//	iov[1].iov_base = str1;
//	iov[1].iov_len = strlen(str1);
//
//	nwritten = writev(STDOUT_FILENO, iov, 2);
}
