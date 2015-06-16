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
#include <boost/thread/mutex.hpp>
#include "chat_message.hpp"





using boost::asio::ip::tcp;




	static std::vector<std::string> message_log;
	static boost::mutex message_log_mutex;
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

	
	
class chat_client
{
private:
	boost::asio::io_service& io_service_;
	tcp::socket socket_;
	ChatMessage read_msg_;
	std::deque<ChatMessage> write_msgs_;

public:
	chat_client(boost::asio::io_service& io_service, tcp::resolver::iterator endpoint_iterator)
		: io_service_(io_service)
		, socket_(io_service)
	{
		do_connect(endpoint_iterator);
	}

  void write(const ChatMessage& msg)
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
  void do_connect(tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
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
        boost::asio::buffer(read_msg_.data(), ChatMessage::header_length),
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



	void send_network_message(chat_client &c, char* line)
	{
		ChatMessage message;
		message.body_length(std::strlen(line));
		std::memcpy(message.body(), line, message.body_length());
		message.encode_header();

		c.write(message);
	}



static boost::asio::io_service GLOBAL__io_service;



class __NetworkServiceINTERNAL
{
private:
	boost::asio::io_service &io_service_loc;
	chat_client *client;
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
	void send_message(char* line)
	{
		if (!client) std::cerr << "Not connected." << std::endl;
		send_network_message(*client, line);
	}
	void connect(std::string url_or_ip, std::string port_num)
	{
		char *argv1 = new char[url_or_ip.length() + 1];
		strcpy(argv1, url_or_ip.c_str());

		char *argv2 = new char[port_num.length() + 1];
		strcpy(argv2, port_num.c_str());

	    tcp::resolver resolver(io_service_loc);
	    auto endpoint_iterator = resolver.resolve({ argv1, argv2 });

		client = new chat_client(io_service_loc, endpoint_iterator);
	    //thread = new std::thread([&io_service_loc](){ io_service_loc.run(); });
	    thread = new std::thread([this](){ io_service_loc.run(); });

		connected = true;

		delete[] argv1;
		delete[] argv2;
	}
	void disconnect()
	{
		std::cout << "Disconnecting...";
		client->close();
		thread->join();
		std::cout << "done." << std::endl;

		connected = false;

		client = 0;
	}
};


#include "network_service.hpp"

/*
class NetworkService
{
private:
	__NetworkServiceINTERNAL *_service;

public:

};
*/


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


void NetworkService::connect(std::string url_or_ip, std::string port_num)
{
	_service->connect(url_or_ip, port_num);
}


void NetworkService::disconnect()
{
	_service->disconnect();
}


void NetworkService::send_message(std::string message)
{
	if (message.length() < ChatMessage::max_body_length)
		message.resize(ChatMessage::max_body_length);
	char *argv1 = new char[message.length() + 1];
	strcpy(argv1, message.c_str());

	_service->send_message(argv1);

	delete[] argv1;
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

