#include "../include/utils.h"
#include <glib.h>
#include <stdio.h>

// NOLINTNEXTLINE
void lupus_strcpy(char const *source, char const **destination) {
    if (!source) {
        g_error("lupus_strcpy: source is null.");
    }

    if (destination) {
        g_free((gpointer)destination);
    }

    *destination = g_strdup(source); // NOLINT
}