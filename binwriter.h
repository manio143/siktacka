#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <string.h>

#include <arpa/inet.h>
#define htonll(x)    \
    ((1 == htonl(1)) \
         ? (x)       \
         : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x)    \
    ((1 == ntohl(1)) \
         ? (x)       \
         : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#include "crc32.h"

class BinaryWriter {
   private:
    std::vector<char> buffer;

   public:
    BinaryWriter() {}

    void write8(uint8_t x) { buffer.push_back(x); }
    void write32(uint32_t x) {
        auto currentSize = buffer.size();
        buffer.resize(currentSize + sizeof(uint32_t));
        *((uint32_t*)&(buffer[currentSize])) = htonl(x);
    }
    void write64(uint64_t x) {
        auto currentSize = buffer.size();
        buffer.resize(currentSize + sizeof(uint64_t));
        *((uint64_t*)&(buffer[currentSize])) = htonll(x);
    }
    void writeString(std::string s) {
        for (int i = 0; i < s.length(); i++)
            buffer.push_back(s[i]);
    }

    void writeCRC() {
        auto len = buffer.size();
        auto crc = crc32(&buffer[0], len);
        write32(crc);
    }

    char* copyTo(char* buff) {
        return (char*)memcpy(buff, &buffer[0], buffer.size()) + buffer.size();
    }
    size_t size() { return buffer.size(); }
};

class BinaryReader {
   private:
    std::vector<char> buffer;
    int pos = 0;

   public:
    BinaryReader(char* buff, size_t size) {
        buffer.resize(size);
        memcpy(&buffer[0], buff, size);
    }

    uint8_t read8() { return (uint8_t)buffer[pos++]; }
    uint32_t read32() {
        uint32_t x;
        x = *((uint32_t*)&(buffer[pos]));
        x = ntohl(x);
        pos += sizeof(uint32_t);
        return x;
    }
    uint64_t read64() {
        uint64_t x;
        x = *((uint64_t*)&(buffer[pos]));
        x = ntohll(x);
        pos += sizeof(uint64_t);
        return x;
    }

    std::string readString(size_t size) {
        std::string s;
        for (int i = 0; i < size; i++) {
            char c = buffer[pos + i];
            s.append(1, c);
        }
        pos += size;
        return s;
    }

    std::string readNullString() {
        std::string s;
        while (buffer[pos++] != 0) {
            char c = buffer[pos - 1];
            s.append(1, c);
        }
        return s;
    }

    // The last 4 bytes are the CRC
    bool checkCRC() {
        int currPos = pos;
        pos = buffer.size() - sizeof(uint32_t);
        auto crc = read32();
        pos = currPos;

        return crc == crc32(&buffer[0], buffer.size() - sizeof(uint32_t));
    }
};