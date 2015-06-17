




#ifndef __AF_NETWORK_SERVICE_HEADER
#define __AF_NETWORK_SERVICE_HEADER




#include <string>

class __NetworkServiceINTERNAL;



// Though not implemented, 
// only one instance of NetworkService should be used

class NetworkService
{
private:
	__NetworkServiceINTERNAL *_service;

public:
	enum { max_message_length = 512 };
	
	NetworkService();
	~NetworkService();

	bool connect(std::string url_or_ip, std::string port_num);
	bool disconnect();

	bool send_message(std::string message);
	virtual void on_message_receive(std::string message);
};




#endif



