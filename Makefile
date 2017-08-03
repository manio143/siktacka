CMD = g++
CFLAGS  = -g -Wall -std=c++11 -DDEBUG

OBJ = arguments.o connectors.o events.o messages.o sock.o err.o crc32.o

all: siktacka-server siktacka-client

siktacka-server: server.cc $(OBJ)
	$(CMD) $(CFLAGS) server.cc $(OBJ) -o siktacka-server

siktacka-client: client.cc $(OBJ)
	$(CMD) $(CFLAGS) client.cc $(OBJ) -o siktacka-client

.cc.o:
	$(CMD) $(CFLAGS) -c $< -o $@

clean: 
	$(RM) siktacka-server siktacka-client *.o *~
