

#ifndef PROTOCOL
#define PROTOCOL

#include "connection.h"
class Connection;
typedef std::weak_ptr<Connection> ConnectionWeak_ptr;
class Protocol : public std::enable_shared_from_this<Protocol>
{	
	// This class is constructed here, inherited in lua.
	// The matter of those are for opening ports.
	public:
		static String name;
		static bool server_sends_first;
};

#endif