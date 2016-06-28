
#include "pch.h"
#include "luaenv.h"

std::mutex g_loaderLock;
std::condition_variable g_loaderSignal;
std::unique_lock<std::mutex> g_loaderUniqueLock(g_loaderLock);

LuaBinder* g_luabind = new LuaBinder();

int main(int, char**[])
{
	if (!g_luabind->doFile("config/config.lua")) {
		g_luabind->reportLastError();
	}
	
	if (!g_luabind->call()) {
		g_luabind->reportLastError();
	}

	std::cin.get();
	return 1;
}