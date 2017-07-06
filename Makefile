CMD = g++
CFLAGS  = -g -Wall -std=c++11

all: siktacka-server siktacka-client

siktacka-server: server.cc arguments.o connectors.o events.o messages.o sock.o
	$(CMD) $(CFLAGS) server.cc -o siktacka-server

siktacka-client: client.cc arguments.o connectors.o events.o messages.o sock.o
	$(CMD) $(CFLAGS) client.cc -o siktacka-client

.cc.o:
	$(CMD) $(CFLAGS) -c $< -o $@

clean: 
	$(RM) siktacka-server siktacka-client *.o *~
