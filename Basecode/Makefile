client : Client.cpp Message.o
	g++ -c Client.cpp -g
	g++ ./Client.o ./Message.o ./EPOLL_MINE.o -lboost_serialization -o client

server : Server.cpp Message.o
	g++ -c -g Server.cpp
	g++ ./Server.o ./Message.o ./EPOLL_MINE.o -lboost_serialization -o server

Message : Message.cpp
	g++ -c Message.cpp

EPOLL_MINE : EPOLLMINE.cpp
	g++ -c EPOLLMINE.cpp

clean : 
	rm client server

all : Message.o EPOLL_MINE.o Client.cpp Server.cpp
	g++ -c Client.cpp -g
	g++ ./Client.o ./Message.o ./EPOLL_MINE.o -lboost_serialization -o client
	g++ -c -g Server.cpp
	g++ ./Server.o ./Message.o ./EPOLL_MINE.o -lboost_serialization -o server
