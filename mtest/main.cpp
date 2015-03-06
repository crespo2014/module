/*
 * main.cpp
 *
 *  Created on: 6 Mar 2015
 *      Author: lester
 */

#include <CppUTest/CommandLineTestRunner.h>

int main(int ac, char** av)
{
    MemoryLeakWarningPlugin::turnOnNewDeleteOverloads();
    return CommandLineTestRunner::RunAllTests(ac, av);
}

