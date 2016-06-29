
#include "pch.h"

#include "scheduler.h"
#include "luaenv.h"
#include "tasks.h"
#include "server.h"

LuaBinder g_luabind;
Dispatcher g_dispatcher;
Scheduler g_scheduler;

std::mutex g_loaderLock;
std::condition_variable g_loaderSignal;
std::unique_lock<std::mutex> g_loaderUniqueLock(g_loaderLock);

void badAllocationHandler()
{
	// Use functions that only use stack allocation
	puts("Allocation failed, server out of memory.\nDecrease the size of your map or compile in 64 bits mode.\n");
	getchar();
	exit(-1);
}

int main(int, char*[])
{
	// Setup bad allocation handler
	std::set_new_handler(badAllocationHandler);

#ifndef _WIN32
	// ignore sigpipe...
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;
	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, nullptr);
#endif

	if (!g_luabind.doFile("config/config.lua")) {
		g_luabind.reportLastError();
	}

	std::cin.get(); // temporary wait.
	return 1;
}