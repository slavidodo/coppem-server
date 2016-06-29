

#ifndef SERVER
#define SERVER

class ServiceManager;
class ServiceBase;
typedef std::shared_ptr<ServiceBase> Service_ptr;
class ServicePort;
typedef std::shared_ptr<ServicePort> ServicePort_ptr;

class Server {
public:
	static void Shutdown(int _code = EXIT_SUCCESS);

	ServiceManager* serviceManager;
	static uint32_t IP;
	static String IPString;
	static String Name;

};

class ServicePort : public std::enable_shared_from_this<ServicePort>, private boost::noncopyable
{
public:
};

class ServiceManager
{
public:
	template <typename ProtocolType>
	bool add(uint16_t port);

	bool is_running() const {
		return acceptors.empty() == false;
	}
protected:
	std::unordered_map<uint16_t, ServicePort_ptr> acceptors;

	boost::asio::io_service io_service;
	boost::asio::deadline_timer death_timer;
	bool running;
};

template <typename ProtocolType>
bool ServiceManager::add(uint16_t port)
{
	if (port == 0) {
		std::cout << "ERROR: No port provided for service " << ProtocolType::name << ". Service disabled." << std::endl;
		return false;
	}

	ServicePort_ptr service_port;

	auto foundServicePort = acceptors.find(port);

	if (foundServicePort == acceptors.end()) {
		service_port = std::make_shared<ServicePort>(io_service);
		service_port->open(port);
		acceptors[port] = service_port;
	} else {
		service_port = foundServicePort->second;

		if (service_port->is_single_socket() || ProtocolType::server_sends_first) {
			std::cout << "ERROR: " << ProtocolType::protocol_name() <<
				" and " << service_port->get_protocol_names() <<
				" cannot use the same port " << port << '.' << std::endl;
			return false;
		}
	}

	return service_port->add_service(std::make_shared<Service<ProtocolType>>());
}

#endif