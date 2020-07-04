#include "include/lupus.h"
#include <cairo.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <sodium/utils.h>
#include <stdlib.h>
#include <tox/tox.h>

#define COLOR_BYTES 6
#define COLORS 2
#define ROWS 5
#define COLS 5

gfloat hue_to_rgb(float p, float q, float t)
{
    if (t < 0) {
        ++t;
    }

    if (t > 1) {
        --t;
    }

    if (t < 1 / 6.0f) {
        return p + (q - p) * 6.0f * t;
    }

    if (t < 1 / 2.0f) {
        return q;
    }

    if (t < 2 / 3.0f) {
        return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    }

    return p;
}

GdkRGBA hsl_to_rgba(float h, float s, float l)
{
    gfloat q = l < 0.5f ? l * (s + 1) : l + s - l * s;
    gfloat p = l * 2 - q;

    return (GdkRGBA){
        .red = hue_to_rgb(p, q, h + 1 / 3.0f),
        .green = hue_to_rgb(p, q, h),
        .blue = hue_to_rgb(p, q, h - 1 / 3.0f),
        .alpha = 1.0f,
    };
}

void load_tox_identicon(guint8 *public_key, gchar *public_key_hex, gsize avatar_size)
{
    gchar *checksum = g_compute_checksum_for_data(G_CHECKSUM_SHA256, public_key, tox_public_key_size());
    gsize checksum_length = strlen(checksum);
    guint8 hash[checksum_length / 2];
    sodium_hex2bin(hash, sizeof(hash), checksum, checksum_length, NULL, 0, NULL);
    g_free(checksum);

    GdkRGBA colors[COLORS] = {0};
    for (gint i = 0; i < COLORS; ++i) {
        guint8 *hash_part = hash + (tox_public_key_size() - (COLOR_BYTES * (i + 1)) - 1);

        guint64 hue_uint = hash_part[0];
        for (gint i = 1; i < COLOR_BYTES; ++i) {
            hue_uint <<= 8;
            hue_uint += hash_part[i];
        }

        gfloat hue = (gfloat)hue_uint / (((guint64)1 << (8 * COLOR_BYTES)) - 1);
        gfloat saturation = 0.5f;
        gfloat light = (i == 0) ? 0.3f : (i == 1) ? 0.8f : 0.55f;

        colors[i] = hsl_to_rgba(hue, saturation, light);
    }

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, avatar_size, avatar_size);
    cairo_t *context = cairo_create(surface);

    for (gint row = 0; row < ROWS; ++row) {
        for (gint col = 0; col < COLS; ++col) {
            gint col_index = abs(((col * 2) - 4) / 2);
            gint pos = row * 3 + col_index;

            GdkRGBA color = colors[hash[pos] % COLORS];
            cairo_set_source_rgb(context, color.red, color.green, color.blue);

            gfloat row_multiplier = (gfloat)avatar_size / ROWS;
            gfloat col_multiplier = (gfloat)avatar_size / COLS;

            cairo_rectangle(context, col * col_multiplier, row * row_multiplier, col_multiplier, row_multiplier);
            cairo_fill(context);
        }
    }

    gchar *avatar_directory = g_strconcat(LUPUS_TOX_DIR, "avatars/", NULL);
    if (!g_file_test(avatar_directory, G_FILE_TEST_IS_DIR)) {
        if (g_mkdir(avatar_directory, 755)) {
            lupus_error("Cannot create avatars directory\n<b>%s</b>", avatar_directory);
            g_free(avatar_directory);
            cairo_destroy(context);
            cairo_surface_destroy(surface);
        }
    }

    gsize avatar_filename_size = strlen(avatar_directory) + tox_public_key_size() * 2 + 4 + 1;
    gchar avatar_filename[avatar_filename_size];
    memset(avatar_filename, 0, avatar_filename_size);
    g_strlcat(avatar_filename, avatar_directory, avatar_filename_size);
    g_strlcat(avatar_filename, public_key_hex, avatar_filename_size);
    g_strlcat(avatar_filename, ".png", avatar_filename_size);
    g_free(avatar_directory);

    cairo_surface_write_to_png(surface, avatar_filename);
    cairo_destroy(context);
    cairo_surface_destroy(surface);
}

