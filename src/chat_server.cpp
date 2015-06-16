#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "chat_message.hpp"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

class ChatUser
{
public:
	virtual ~ChatUser() {}
	virtual void deliver(const ChatMessage& msg) = 0;
};

typedef std::shared_ptr<ChatUser> ChatUser_ptr;

//----------------------------------------------------------------------

class ChatRoom
{
private:
	std::set<ChatUser_ptr> users;
	int max_recent_msgs;
	std::deque<ChatMessage> recent_msgs_;

public:
	ChatRoom()
		: max_recent_msgs(100)
	{}
	void join(ChatUser_ptr user)
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

	void leave(ChatUser_ptr user)
	{
		users.erase(user);
		std::cout << "Participant (" << user << ") left" << std::endl;
	}

	void deliver(const ChatMessage& msg)
	{
		recent_msgs_.push_back(msg);

		while (recent_msgs_.size() > max_recent_msgs)
			recent_msgs_.pop_front();

		for (auto user: users)
			user->deliver(msg);
	}
};

//---------------------------------------------------------------------

class ChatUserSession : public ChatUser, public std::enable_shared_from_this<ChatUserSession>
{
private:
	tcp::socket socket_;
	ChatRoom& room_;
	ChatMessage read_msg_;
	std::deque<ChatMessage> write_msgs_;

public:
	ChatUserSession(tcp::socket socket, ChatRoom& room)
		: socket_(std::move(socket))
		, room_(room)
	{
		std::cout << "starting chat session" << std::endl;
	}

	void start()
	{
		room_.join(shared_from_this());
		do_read_header();
	}

	void deliver(const ChatMessage& msg)
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
    boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), ChatMessage::header_length),
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

class ChatServer
{
private:
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	ChatRoom room_;

public:
	ChatServer(boost::asio::io_service& io_service, const tcp::endpoint& endpoint)
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
		    std::make_shared<ChatUserSession>(std::move(socket_), room_)->start();
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
      std::cerr << "Usage: ChatServer <port1> [<port2> ...]\n";
      return 1;
    }

    boost::asio::io_service io_service;

    std::list<ChatServer> servers;
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
