#include "../include/utils.h"
#include "../include/lupus.h"
#include "toxencryptsave/toxencryptsave.h"

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

void save_tox(Tox *tox, char const *profile_name, char const *password, gpointer user_data) {
    size_t content_size = tox_get_savedata_size(tox);
    uint8_t *content = g_malloc(content_size);
    tox_get_savedata(tox, content);

    if (password && strlen(password) > 0) {
        uint8_t *content_encrypted = g_malloc(content_size + TOX_PASS_ENCRYPTION_EXTRA_LENGTH);

        TOX_ERR_ENCRYPTION tox_err_encryption;
        tox_pass_encrypt(
                content, content_size,
                (uint8_t *) password, strlen(password),
                content_encrypted,
                &tox_err_encryption);
        if (tox_err_encryption != TOX_ERR_ENCRYPTION_OK) {
            error_message(user_data, "<b>Error</b>: tox_pass_encrypt returned %d.", tox_err_encryption);
            g_free(content);
            g_free(content_encrypted);
            return;
        }

        g_free(content);
        content = content_encrypted;
        content_size += TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    char *tox_directory = LUPUS_TOX_DIR;
    if (!g_file_test(tox_directory, G_FILE_TEST_IS_DIR)) {
        error_message(user_data, "<b>Error</b>: directory %s does not exists.", tox_directory);
        g_free(content);
        g_free(tox_directory);
        return;
    }

    char *profile_filename = g_strconcat(tox_directory, profile_name, ".tox", NULL);
    g_free(tox_directory);

    GError *error = NULL;
    g_file_set_contents(profile_filename, (gchar *) content, content_size, &error);
    if (error != NULL) {
        error_message(user_data, "<b>Error</b>: %d<br>%s.", error->code, error->message);
    }

    g_free(content);
    g_free(profile_filename);
    if (error != NULL) {
        g_error_free(error);
    }
}

void error_message(GtkWindow *parent, char const *format, ...) {
    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);

    size_t message_length = vsnprintf(NULL, 0, format, args_copy);
    char *message = g_malloc(message_length) + 1;
    vsprintf(message, format, args);

    GtkWidget *dialog = gtk_message_dialog_new(
            parent,
            GTK_DIALOG_USE_HEADER_BAR,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            NULL);
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    va_end(args_copy);
    va_end(args);
}
