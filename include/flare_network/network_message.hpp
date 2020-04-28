#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

class NetworkMessage
{
public:
  enum { header_length = 4 };
  enum { max_body_length = 512 };

  NetworkMessage()
    : body_length_(0)
	, recipient_id(0)
	, sender_id(0)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  std::size_t length() const
  {
    return header_length + body_length_;
  }

  const char* body() const
  {
    return data_ + header_length;
  }

  char* body()
  {
    return data_ + header_length;
  }

  std::size_t body_length() const
  {
    return body_length_;
  }

  void body_length(std::size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  bool decode_header()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    body_length_ = std::atoi(header);
    if (body_length_ > max_body_length)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    char header[header_length + 1] = "";
    std::sprintf(header, "%4d", static_cast<int>(body_length_));
    std::memcpy(data_, header, header_length);
  }

  int get_recipient_id() const
  {
	  return recipient_id;
  }

  void set_recipient_id(int intended_recipient_id)
  {
	  recipient_id = intended_recipient_id;
  }

  int get_sender_id() const
  {
	  return sender_id;
  }

  void set_sender_id(int new_sender_id)
  {
	  sender_id = new_sender_id;
  }

private:
  char data_[header_length + max_body_length];
  std::size_t body_length_;
  int sender_id;
  int recipient_id;
};
