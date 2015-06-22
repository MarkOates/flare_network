#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "network_message.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

class NetworkUser
{
public:
	virtual ~NetworkUser() {}
	virtual void deliver(const NetworkMessage& msg) = 0;
};

typedef std::shared_ptr<NetworkUser> ChatUser_ptr;

//----------------------------------------------------------------------

class NetworkRoom
{
private:
	std::set<NetworkUser_ptr> users;
	int max_recent_msgs;
	std::deque<NetworkMessage> recent_msgs_;

public:
	NetworkRoom()
		: max_recent_msgs(100)
	{}
	void join(NetworkUser_ptr user)
	{
		users.insert(user);
		/*
		// this line dumps recent messages when a new user joins.
		// not a useful thing, nobody does this.
	    for (auto msg: recent_msgs_)
	      participant->deliver(msg);
		*/
		std::cout << "Participant (" << user << ") joined!" << std::endl;
	}

	void leave(NetworkUser_ptr user)
	{
		users.erase(user);
		std::cout << "Participant (" << user << ") left" << std::endl;
	}

	void deliver(const NetworkMessage& msg)
	{
		recent_msgs_.push_back(msg);

		while (recent_msgs_.size() > max_recent_msgs)
			recent_msgs_.pop_front();

		for (auto user: users)
			user->deliver(msg);
	}
};

//---------------------------------------------------------------------

class NetworkUserSession : public ChatUser, public std::enable_shared_from_this<ChatUserSession>
{
private:
	tcp::socket socket_;
	NetworkRoom& room_;
	NetworkMessage read_msg_;
	std::deque<NetworkMessage> write_msgs_;

public:
	NetworkUserSession(tcp::socket socket, ChatRoom& room)
		: socket_(std::move(socket))
		, room_(room)
	{
		std::cout << "starting session" << std::endl;
	}

	void start()
	{
		room_.join(shared_from_this());
		do_read_header();
	}

	void deliver(const NetworkMessage& msg)
	{
		bool write_in_progress = !write_msgs_.empty();
		write_msgs_.push_back(msg);
		if (!write_in_progress)
		{
			do_write();
		}
	}

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), NetworkMessage::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            room_.deliver(read_msg_);
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
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
            room_.leave(shared_from_this());
          }
        });
  }
};

//----------------------------------------------------------------------

class NetworkServer
{
private:
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	NetworkRoom room_;

public:
	NetworkServer(boost::asio::io_service& io_service, const tcp::endpoint& endpoint)
		: acceptor_(io_service, endpoint)
		, socket_(io_service)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
		{
		  if (!ec)
		  {
		    std::make_shared<NetworkUserSession>(std::move(socket_), room_)->start();
		  }

		  do_accept();
		});
	}
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: NetworkServer <port1> [<port2> ...]\n";
      return 1;
    }

    boost::asio::io_service io_service;

    std::list<NetworkServer> servers;
    for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_service, endpoint);
    }

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
