// crc32.h

#include <unistd.h>
#include <stdint.h>

#ifndef M_CRC32
#define M_CRC32

uint32_t crc32(char* msg, int len);

#endif