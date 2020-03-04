#include "../include/utils.h"
#include "../include/lupus.h"
#include <toxencryptsave/toxencryptsave.h>

// FIXME: write to temp file for safety
gboolean tox_save(Tox *tox, gchar const *filename, gchar const *password,
              GtkWindow *window, gboolean new) {
    gsize savedata_size = tox_get_savedata_size(tox);
    guint8 *savedata = g_malloc(savedata_size);
    tox_get_savedata(tox, savedata);

    /* if password is set and is not empty */
    if (password && *password) {
        guint8 *tmp =
            g_malloc(savedata_size + TOX_PASS_ENCRYPTION_EXTRA_LENGTH);

        if (!tox_pass_encrypt(savedata, savedata_size, (guint8 *)password,
                              strlen(password), tmp, NULL)) {
            lupus_error(window, "Cannot encrypt profile.");
            g_free(tmp);
            g_free(savedata);
            return FALSE;
        }

        g_free(savedata);
        savedata = tmp;
        savedata_size += TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    GError *error = NULL;
    g_file_set_contents(filename, (gchar *)savedata, savedata_size, &error);
    if (error) {
        lupus_error(window, "Cannot save profile: %s", error->message);
        g_error_free(error);
        g_free(savedata);
        return FALSE;
    }

    if (new) {
        lupus_success(window, "Profile \"%s\" created.", filename);
    }

    g_free(savedata);
    return TRUE;
}