#ifndef LUPUS_UTILS_H
#define LUPUS_UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "toxcore/tox.h"

uint8_t *hex2bin(char const *hex);

char *bin2hex(uint8_t const *bin, size_t length);

void save_tox(Tox *tox, char const *profile_name, char const *password, gpointer user_data);

void error_message(GtkWindow *parent, char const *format, ...);

#endif