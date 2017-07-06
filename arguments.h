#pragma once

#include <stdint.h>
#include <string>

class ServerArguments {
   private:
    void setDefaults();

   public:
    uint32_t width;            // 800
    uint32_t height;           // 600
    uint16_t port;             // 12345
    uint32_t roundsPerSecond;  // 50
    uint32_t turningSpeed;     // 6
    uint64_t randomSeed;       // time(NULL)

    ServerArguments(int argc, char** argv);
};

class ClientArguments {
   public:
    std::string playerName;
    char* host;
    uint16_t port;
    char* guihost;
    uint16_t guiport;

    ClientArguments(int argc, char** argv);
};