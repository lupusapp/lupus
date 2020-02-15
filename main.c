#include "include/lupus.h"
#include "include/lupus_application.h"

char *LUPUS_TOX_DIR;

gint main(gint argc, gchar **argv) {
    gchar const *config_dir;

    g_return_val_if_fail((config_dir = g_get_user_config_dir()), 1);

    LUPUS_TOX_DIR = g_strconcat(config_dir, "/tox/", NULL);
    g_return_val_if_fail(
        g_file_test(LUPUS_TOX_DIR, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR), // NOLINT
        1);

    gint res =
        g_application_run(G_APPLICATION(lupus_application_new()), argc, argv);
    g_free(LUPUS_TOX_DIR);

    return res;
}