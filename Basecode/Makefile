client : Client.cpp Message.o
	g++ -c Client.cpp -g
	g++ ./Client.o ./Message.o -lboost_serialization -o client

server : Server.cpp Message.o
	g++ -c -g Server.cpp
	g++ ./Server.o ./Message.o -lboost_serialization -o server

Message : Message.cpp
	g++ -c Message.cpp

clean : 
	rm client server