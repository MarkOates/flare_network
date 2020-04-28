

#CC=x86_64-w64-mingw32-g++
CC=g++


ifeq ($(OS),Windows_NT)
	REQUIRED_WINDOWS_FLAGS=-lwsock32 -lws2_32
endif


all:
	@echo Usage: \"make server\" OR \"make client_ex\"


server:
	$(CC) -std=gnu++11 ./src/chat_server.cpp -o ./bin/chat_server -I../asio/asio/include -I/usr/include -I./include $(REQUIRED_WINDOWS_FLAGS)

client_ex:
	$(CC) -std=gnu++11 ./src/chat_client.cpp ./examples/chat_client_ex.cpp -o ./bin/chat_client_ex -I../asio/asio/include -I/usr/include -I./include $(REQUIRED_WINDOWS_FLAGS)


