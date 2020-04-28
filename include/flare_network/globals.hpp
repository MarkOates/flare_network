#pragma once

#include <functional>
#include <asio.hpp>
extern std::function<void(std::string)> _on_recieve_message_callback_func;
extern asio::io_service GLOBAL__io_service;

