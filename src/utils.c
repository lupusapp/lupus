#include "../include/utils.h"
#include <glib.h>

uint8_t *hex2bin(char const *hex) {
    size_t len, i;
    uint8_t *bin;

    len = strlen(hex) / 2;
    bin = g_malloc(len);

    for (i = 0; i < len; ++i) {
        sscanf(hex, "%2hhx", &bin[i]);
    }

    return bin;
}

char *bin2hex(uint8_t const *bin, size_t length) {
    char *hex;
    char *hex_start;
    int i;

    hex = g_malloc(2 * length + 1);
    hex_start = hex;

    for (i = 0; i < length; ++i, hex += 2) {
        sprintf(hex, "%02X", bin[i]);
    }

    return hex_start;
}