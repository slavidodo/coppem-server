

#include "pch.h"
#include "connection.h"
#include "networkmessage.h"

#include "server.h"
#include "tasks.h"
#include "scheduler.h"

extern Dispatcher g_dispatcher;
extern Scheduler g_scheduler;

void Server::Shutdown(int _code)
{
	std::cout << "Shutting down..." << std::flush;

	g_dispatcher.shutdown();
	g_scheduler.shutdown();

	std::exit(_code); // force exit
}

///////