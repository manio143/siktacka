all: siktacka-server siktacka-client

siktacka-server: server.cpp types.h net.h const.h hton.h err.h
	g++ -Wall -g -std=c++11 server.cpp -o siktacka-server

siktacka-client: client.cpp types.h net.h const.h hton.h err.h
	g++ -Wall -g -std=c++11 client.cpp -o siktacka-client
