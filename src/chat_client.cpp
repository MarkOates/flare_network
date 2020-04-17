#ifdef _WIN32
// these defines are used when building with boost 1.56.0
#define _WIN32_WINNT 0x0601
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
//#include <boost/phoenix/bind/bind_member_function.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
//#include <boost/thread/mutex.hpp>
#include <mutex>
#include "network_message.hpp"





//using boost::asio::ip::tcp;




static std::vector<std::string> message_log;
//static boost::mutex message_log_mutex;
static std::mutex message_log_mutex;
static int num_new_messages = 0;
boost::function<void(std::string)> _on_recieve_message_callback_func;
//static void (*_on_recieve_message_callback_func)(std::string) = 0; 

void write_log_message(std::string message)
{
	message_log.push_back(message);
	num_new_messages++;
	//if (_on_recieve_message_callback_func)
	//	_on_recieve_message_callback_func(message);
	_on_recieve_message_callback_func(message);
}





class NetworkClient
{
private:
	boost::asio::io_service& io_service_;
   boost::asio::ip::tcp::socket socket_;
	NetworkMessage read_msg_;
	std::deque<NetworkMessage> write_msgs_;

public:
	NetworkClient(boost::asio::io_service& io_service, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
		: io_service_(io_service)
		, socket_(io_service)
	{
		do_connect(endpoint_iterator);
	}

  void write(const NetworkMessage& msg)
  {
    io_service_.post(
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

	void close()
	{
		io_service_.post([this]() { socket_.close(); });
	}

private:
  void do_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), NetworkMessage::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_read_body()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
		message_log_mutex.lock();
		//message_log_mutex.lock();
		std::string message_text(read_msg_.body(), read_msg_.body_length());
		write_log_message(message_text);
		message_log_mutex.unlock();
		//message_log_mutex.unlock();
		//std::cout.write(read_msg_.body(), read_msg_.body_length());
		//std::cout << "\n";

		do_read_header();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_write()
  {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            socket_.close();
          }
        });
  }
};



void send_network_message(NetworkClient &c, char* line)
{
	NetworkMessage message;
	message.body_length(std::strlen(line));
	std::memcpy(message.body(), line, message.body_length());
	message.encode_header();

	c.write(message);
}



// this is global (but static).  This should be fixed
static boost::asio::io_service GLOBAL__io_service;



class __NetworkServiceINTERNAL
{
private:
	boost::asio::io_service &io_service_loc;
	NetworkClient *client;
	std::thread *thread;
	bool connected;

public:
	__NetworkServiceINTERNAL()
		: io_service_loc(GLOBAL__io_service)
		, client(0)
		, thread(0)
		, connected(false)
	{
	}
	bool is_connected()
	{
		return connected;
	}
	bool send_message(char* line)
	{
		if (!connected) return false;
		send_network_message(*client, line);
		return true;
	}
	bool connect(std::string url_or_ip, std::string port_num)
	{
		if (connected) return false;

		char *argv1 = new char[url_or_ip.length() + 1];
		strcpy(argv1, url_or_ip.c_str());

		char *argv2 = new char[port_num.length() + 1];
		strcpy(argv2, port_num.c_str());

      boost::asio::ip::tcp::resolver resolver(io_service_loc);
	    auto endpoint_iterator = resolver.resolve({ argv1, argv2 });

		client = new NetworkClient(io_service_loc, endpoint_iterator);
	    //thread = new std::thread([&io_service_loc](){ io_service_loc.run(); });
	    thread = new std::thread([this](){
			if (io_service_loc.stopped()) io_service_loc.reset(); // reset is required if the io_service has stopped
			io_service_loc.run();
			std::cout << "disconnection event occured" << std::endl;

			//disconnect(); // this function call is a little sloppy here.  It's purpose is to call disconnect() if the
						  // disconnection was initiated from the server
		});

		connected = true;

		delete[] argv1;
		delete[] argv2;

		return true;
	}
	bool disconnect()
	{
		if (!connected) return false;

		std::cout << "Disconnecting...";
		client->close();
		thread->join();
		std::cout << "done." << std::endl;

		connected = false;
		delete thread;
		delete client;
		client = 0; // < ?? should client be deleted?
		thread = 0; // < ?? should thread be deleted?

		return true;
	}
};





#include <flare_network/network_service.hpp>


NetworkService::NetworkService()
	: _service(0)
{
	_service = new __NetworkServiceINTERNAL();
	//_on_recieve_message_callback_func = callback_func_ex;
	_on_recieve_message_callback_func = boost::bind(&NetworkService::on_message_receive, this, _1);
}


NetworkService::~NetworkService()
{
	delete _service;
}


bool NetworkService::connect(std::string url_or_ip, std::string port_num)
{
	return _service->connect(url_or_ip, port_num);
}


bool NetworkService::disconnect()
{
	return _service->disconnect();
}


bool NetworkService::is_connected()
{
	return _service->is_connected();
}


bool NetworkService::send_message(std::string message)
{
	if (message.length() < NetworkMessage::max_body_length)
		message.resize(NetworkMessage::max_body_length);
	char *argv1 = new char[message.length() + 1];
	strcpy(argv1, message.c_str());

	bool ret_val = _service->send_message(argv1);

	delete[] argv1;

	return ret_val;
}


void NetworkService::on_message_receive(std::string message)
{
	std::cout << "BOUND>" << message << std::endl;
}





/*

int main(int argc, char* argv[])
{
	std::string ip_or_url = "openvpa.com";
	std::string port_num = "3000";


  try
  {
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
	NetworkServiceINTERNAL *network_service = new NetworkServiceINTERNAL();
	network_service->connect(ip_or_url, port_num);

		char line[ChatMessage::max_body_length + 1];
		bool abort = false;
		while (!abort)
		{
			std::cin.getline(line, ChatMessage::max_body_length + 1);
			if (line[0] == 'q') abort = true;
			else network_service->send_message(line);
		}
	network_service->disconnect();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}


*/

