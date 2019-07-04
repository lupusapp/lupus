#ifndef LUPUS_UTILS_H
#define LUPUS_UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

uint8_t *hex2bin(char const *hex);

char *bin2hex(uint8_t const *bin, size_t length);

#endif