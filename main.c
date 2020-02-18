#include "include/lupus.h"
#include "include/lupus_application.h"

char *LUPUS_TOX_DIR;

gint main(gint argc, gchar **argv) {
    gtk_init(&argc, &argv);

    gchar const *config_dir = g_get_user_config_dir();
    g_return_val_if_fail(config_dir, 1);

    LUPUS_TOX_DIR = g_strconcat(config_dir, "/tox/", NULL);
    g_return_val_if_fail(
        g_file_test(LUPUS_TOX_DIR,
                    G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR), // NOLINT
        1);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(provider, LUPUS_RESOURCES "/lupus.css");
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gint res =
        g_application_run(G_APPLICATION(lupus_application_new()), argc, argv);
    g_free(LUPUS_TOX_DIR);

    return res;
}