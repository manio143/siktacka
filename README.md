# Siktacka
A clone of Netacka (https://pwmarcz.pl/netacka/) I did for my Computer Network course. The idea is simple: we are a snake-like creature and our goal is to survive. We lose if we touch other players or the walls.

The whole task is described in Polish in [zadanie_2/zadanie_2.txt](zadanie_2/zadanie_2.txt).

### Architecture
The game consists of three parts:

1. Server - uses UDP to communicate with the Client and has all the game logic
2. Client - is a bridge between the GUI app and the server
3. GUI - uses TCP to communicate with the client, sending it the keypresses and receiving display instructions - was provided by teacher

The system is object oriented. All C code required for Linux internet connections has been nicely wrapped in the Sock, IPAddress and Connectors classes.

I implemented a simple binary reader/writer to enable easier parsing of messages and producing the bytes from a message object.

### Compiling

    make

To run the game you also have to compile the gui

    cd zadanie2/gui/gui3    # or gui2 for GTK2
    make
  
### Running
First run the server

    ./siktacka-server [-W n] [-H n] [-p n] [-s n] [-t n] [-r n]

Where `-W` and `-H` are the width and height of the board, `-p` is port number (12345 by default), `-s` is the game speed in ROUNDS_PER_SECOND (50 by default), `-t` is the TURNING SPEED (6 by default) and `-r` is the seed for the random generator.

Then run the GUI

    ./zadanie2/gui/gui3/gui [port]
  
Then run the client

    ./siktacka-client  player_name  game_sever_host[:port] [ui_server_host[:port]]
  
Where the default values are 12345 for game server port, `localhost` for ui server and 12346 for its port.

### Playing
To start a game you need at least two players connected to the server. When all players are pressing one of the arrows (sending a turn signal) the game will start.

A player without a name can only be a spectator.
