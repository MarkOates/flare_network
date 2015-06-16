

#CC=x86_64-w64-mingw32-g++
CC=g++


server:
	$(CC) -std=gnu++11 ./src/chat_server.cpp -o chat_server -I/usr/include -lboost_system -pthread


client:
	$(CC) -std=gnu++11 ./src/chat_client.cpp ./src/chat_client_ex.cpp -o chat_client_ex -I/usr/include -lboost_system -pthread


