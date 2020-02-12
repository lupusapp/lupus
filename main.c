#include "include/lupus_application.h"
#include "include/lupus.h"

char *LUPUS_TOX_DIR;

int main(int argc, char **argv) {
    char const* config_dir;

    if (!(config_dir = g_get_user_config_dir())) {
        g_error("Cannot obtain config directory.");
    }

    if (!(LUPUS_TOX_DIR = g_strconcat(config_dir, "/tox/", NULL))) {
        g_error("Cannot obtain tox directory.");
    }

    // NOLINTNEXTLINE
    if (!(g_file_test(LUPUS_TOX_DIR, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))) {
        g_error("Tox directory %s doesn't exist.", LUPUS_TOX_DIR);
    }

    int code = g_application_run(G_APPLICATION(lupus_application_new()), argc, argv);

    g_free(LUPUS_TOX_DIR);

    return code;
}