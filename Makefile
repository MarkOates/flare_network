

#CC=x86_64-w64-mingw32-g++
CC=g++


all:
	@echo Usage: \"make server\" OR \"make client_ex\"


server:
	$(CC) -std=gnu++11 ./src/chat_server.cpp -o ./bin/chat_server -I../asio/asio/include -I/usr/include -I./include -lboost_system -pthread

client_ex:
	$(CC) -std=gnu++11 ./src/chat_client.cpp ./examples/chat_client_ex.cpp -o ./bin/chat_client_ex -I/usr/include -I./include -lboost_system -pthread


