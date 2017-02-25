#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int read_len(FILE *file, int length, char c) {
    int ret = -1;
    int i = 0;
    unsigned char *buffer = (char*)malloc(length);
    if (buffer == NULL)
        return -2;

    size_t size = fread(buffer, 1, length, file);
    if (size == length) {
        for (i = 0; i < size; i++)
            if(c == 'c')
                printf("%c", buffer[i]);
            else if (c == 'x')
                printf("%02x ", buffer[i]);
        ret = size;
    } else {
        printf("Expected %d, read: %d", length, size);
        ret = -3;
    }
    free(buffer);
    return ret;
}

uint8_t read8(FILE *file) {
    unsigned char buffer = 0;
    size_t size = fread(&buffer, 1, 1, file);
    if (size == 1) {
        printf("%02x ", buffer);
    }
    return buffer;
}
uint16_t read16(FILE *file) {
    uint32_t ret = 0;
    unsigned char buffer[2] = {0};
    size_t size = fread(&buffer, 1, 2, file);
    if  (size == 2) {
        ret = buffer[0] << 8*1 | buffer[1];
        printf("%04x", ret);
    }
    return ret;
}
uint32_t read32(FILE *file) {
    uint32_t ret = 0;
    unsigned char buffer[4] = {0};
    size_t size = fread(&buffer, 1, 4, file);
    if  (size == 4) {
        ret = buffer[0] << 8*3 | buffer[1] << 8*2 | buffer[2] << 8*1 | buffer[3];
        printf("%08x", ret);
    }
    return ret;
}
