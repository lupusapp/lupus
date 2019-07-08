#include "../include/utils.h"
#include <glib.h>

uint8_t *hex2bin(char const *hex) {
    size_t len = strlen(hex) / 2;
    uint8_t *bin = g_malloc(len);

    for (size_t i = 0; i < len; ++i) {
        sscanf(hex, "%2hhx", &bin[i]);
    }

    return bin;
}

char *bin2hex(uint8_t const *bin, size_t length) {
    char *hex = g_malloc(2 * length + 1);
    char *hex_start = hex;

    for (int i = 0; i < length; ++i, hex += 2) {
        sprintf(hex, "%02X", bin[i]);
    }

    return hex_start;
}