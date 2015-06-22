#include <flare_network/network_service.hpp>
#include <string>
#include <iostream>



int main(int argc, char* argv[])
{
	try
	{
		std::string ip_or_url = "openvpa.com";
		std::string port_num = "3000";

		if (argc != 3)
		{
			std::cout << "Usage: chat_client <host> <port>\n";
			std::cout << "no values provided, defaulting to openvpa.com 3000" << std::endl;
		}
		else
		{
			ip_or_url = argv[1];
			port_num = argv[2];
		}

		// create the network service object

		NetworkService *network_service = new NetworkService();
		network_service->connect(ip_or_url, port_num);

		// for this example, we'll use the cin to get input from the user

		char line[NetworkService::max_message_length + 1];
		bool abort = false;
		while (!abort)
		{
			std::cin.getline(line, NetworkService::max_message_length + 1);
			if (line[0] == 'q') abort = true;
			else network_service->send_message(line);
		}

		// after it's over, disconnect from the service
		
		network_service->disconnect();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
