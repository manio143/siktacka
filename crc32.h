// crc32.h

#include <unistd.h>

#ifndef M_CRC32
#define M_CRC32

// key is the crc-32 ieee 802.3 standard polynomial
const uint32_t poly = 0x04C11DB7;

uint32_t reflect(uint32_t data, int bits) {
    uint32_t ret = 0;
    for(int i = bits-1; i>=0; i--) {
        if(data & 1) ret |= (1 << i);
        data >>= 1;
    }
    return ret;
}

uint32_t crc32(char* msg, int len) {
    uint32_t crc = 0xffffffff;
    int i, j;
    for(i = 0; i < len; i++) {
        crc ^= ((char)reflect(msg[i], 8) << 24);
        for(j = 8; j; j--) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? poly : 0x0);
        }
    }
    return reflect(crc, 32) ^ 0xffffffff;
}

#endif