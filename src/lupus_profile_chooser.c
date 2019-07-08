#include "toxcore/tox.h"
#include "toxencryptsave/toxencryptsave.h"
#include "../include/lupus_profile_chooser.h"
#include "../include/lupus.h"
#include "../include/utils.h"
#include "../include/lupus_profile_chooser_password_dialog.h"

struct _LupusProfileChooser {
    GtkApplicationWindow parent_instance;
};

typedef struct _LupusProfileChooserPrivate LupusProfileChooserPrivate;
struct _LupusProfileChooserPrivate {
    GtkHeaderBar *header_bar;
    GtkStack *stack;
    GtkBox *login_box, *register_box;
    GtkEntry *register_name, *register_password;
    GtkButton *register_button;
};

G_DEFINE_TYPE_WITH_PRIVATE(LupusProfileChooser, lupus_profile_chooser, GTK_TYPE_APPLICATION_WINDOW)

static void lupus_profile_chooser_init(LupusProfileChooser *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    LupusProfileChooserPrivate *private = lupus_profile_chooser_get_instance_private(instance);
    gtk_header_bar_set_subtitle(private->header_bar, LUPUS_VERSION);

    g_signal_connect(private->stack, "notify::visible-child", G_CALLBACK(stack_change_callback), instance);
    g_signal_connect(private->register_button, "clicked", G_CALLBACK(register_callback), instance);

    list_tox_profile(private->login_box);
}

static void lupus_profile_chooser_class_init(LupusProfileChooserClass *class) {
    gchar *resource = g_strconcat(LUPUS_RESOURCES, "/profile_chooser.ui", NULL);
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), resource);
    g_free(resource);

    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooser, header_bar);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooser, stack);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooser, login_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooser, register_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooser, register_name);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooser, register_password);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooser, register_button);
}

static void stack_change_callback(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    gboolean is_register = g_strcmp0(gtk_stack_get_visible_child_name(GTK_STACK(gobject)), "register") == 0;
    LupusProfileChooserPrivate *priv = lupus_profile_chooser_get_instance_private(LUPUS_PROFILE_CHOOSER(user_data));

    if (is_register == false) {
        list_tox_profile(priv->login_box);
    }

    gtk_widget_show_all(GTK_WIDGET(user_data));
    gtk_container_foreach(
            GTK_CONTAINER(is_register ? priv->login_box : priv->register_box),
            (gpointer) gtk_widget_hide,
            NULL
    );
}

static void register_callback(GtkButton *button, gpointer user_data) {
    TOX_ERR_NEW tox_err_new;
    Tox *tox = tox_new(NULL, &tox_err_new);
    g_assert(tox_err_new == TOX_ERR_NEW_OK);

    TOX_ERR_SET_INFO tox_err_set_info;
    char *user = "Lupus's user";
    tox_self_set_name(tox, (uint8_t *) user, strlen(user), &tox_err_set_info);
    g_assert(tox_err_set_info == TOX_ERR_SET_INFO_OK);
    char *status_message = "Lupus rocks !";
    tox_self_set_status_message(tox, (uint8_t *) status_message, strlen(status_message), &tox_err_set_info);
    g_assert(tox_err_set_info == TOX_ERR_SET_INFO_OK);

    LupusProfileChooserPrivate *priv = lupus_profile_chooser_get_instance_private(LUPUS_PROFILE_CHOOSER(user_data));
    char const *profile_name = gtk_entry_get_text(priv->register_name);
    char const *profile_password = gtk_entry_get_text(priv->register_password);

    char const *tox_directory = LUPUS_TOX_DIR;
    g_assert(g_file_test(tox_directory, G_FILE_TEST_IS_DIR));

    char const *profile_filename = g_strconcat(tox_directory, profile_name, ".tox", NULL);
    g_assert(g_file_test(profile_filename, G_FILE_TEST_EXISTS) == false);

    size_t content_length = tox_get_savedata_size(tox);
    uint8_t *content = g_malloc(content_length);
    tox_get_savedata(tox, content);

    GError *error = NULL;
    if (strlen(profile_password) != 0) {
        uint8_t *content_encrypted = g_malloc(content_length + TOX_PASS_ENCRYPTION_EXTRA_LENGTH);

        TOX_ERR_ENCRYPTION tox_err_encryption;
        tox_pass_encrypt(
                content, content_length,
                (uint8_t *) profile_password, strlen(profile_password),
                content_encrypted,
                &tox_err_encryption
        );

        g_assert(tox_err_encryption == TOX_ERR_ENCRYPTION_OK);
        g_assert(tox_is_data_encrypted(content_encrypted));

        g_free(content);
        content = content_encrypted;
        content_length += TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    g_file_set_contents(profile_filename, (gchar *) content, content_length, &error);

    g_free(content);
    g_assert(error == NULL);

    g_free((gpointer) profile_filename);
    g_free((gpointer) tox_directory);
    if (error != NULL) {
        g_error_free(error);
    }
    tox_kill(tox);
}

static void login_callback(GtkButton *button, gpointer user_data) {
    gchar *tox_directory = LUPUS_TOX_DIR;
    g_file_test(tox_directory, G_FILE_TEST_IS_DIR);

    gchar *filename = g_strconcat(tox_directory, (gchar *) user_data, NULL);
    g_assert(g_file_test(filename, G_FILE_TEST_EXISTS));

    GError *error = NULL;
    gchar *content = NULL;
    gsize content_length;
    g_file_get_contents(filename, &content, &content_length, &error);
    g_assert(error == NULL);
    g_free(filename);
    g_free(tox_directory);
    if (error != NULL) {
        g_error_free(error);
    }

    if (tox_is_data_encrypted((uint8_t *) content)) {
        LupusProfileChooserPasswordDialog *password_dialog = lupus_profile_chooser_password_dialog_new();

        gchar *password = NULL;
        g_signal_connect(password_dialog, "decrypt", G_CALLBACK(set_password_callback), &password);

        if (gtk_dialog_run(GTK_DIALOG(password_dialog)) != GTK_RESPONSE_ACCEPT) {
            gtk_widget_destroy(GTK_WIDGET(password_dialog));
            goto free;
        }

        gtk_widget_destroy(GTK_WIDGET(password_dialog));

        TOX_ERR_DECRYPTION tox_err_decryption;
        uint8_t *content_decrypted = g_malloc(content_length - TOX_PASS_ENCRYPTION_EXTRA_LENGTH);
        tox_pass_decrypt(
                (uint8_t *) content, content_length,
                (uint8_t *) password, strlen(password),
                content_decrypted,
                &tox_err_decryption
        );
        g_assert(tox_err_decryption == TOX_ERR_DECRYPTION_OK);

        g_free(content);
        content = (char *) content_decrypted;
        content_length -= TOX_PASS_ENCRYPTION_EXTRA_LENGTH;

        g_free(password);
    }

    TOX_ERR_OPTIONS_NEW tox_err_options_new;
    struct Tox_Options *options = tox_options_new(&tox_err_options_new);
    g_assert(tox_err_options_new == TOX_ERR_OPTIONS_NEW_OK);
    tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);
    tox_options_set_savedata_data(options, (uint8_t *) content, content_length);

    TOX_ERR_NEW tox_err_new;
    Tox *tox = tox_new(options, &tox_err_new);
    g_assert(tox_err_new == TOX_ERR_NEW_OK);
free:
    tox_options_free(options);
    g_free(content);
}

void list_tox_profile(GtkBox *login_box) {
    gtk_container_foreach(GTK_CONTAINER(login_box), (gpointer) gtk_widget_destroy, NULL);

    gchar *tox_directory = LUPUS_TOX_DIR;
    g_assert(g_file_test(tox_directory, G_FILE_TEST_IS_DIR));

    GError *error = NULL;
    GDir *tox_files = g_dir_open(tox_directory, 0, &error);
    g_assert(error == NULL);
    if (error != NULL) {
        g_error_free(error);
    }

    gchar *filename;
    GPtrArray *profiles = g_ptr_array_new();
    while ((filename = (gchar *) g_dir_read_name(tox_files))) {
        if (g_str_has_suffix(filename, ".tox")) {
            g_ptr_array_add(profiles, filename);
        }
    }
    g_dir_close(tox_files);
    g_free(tox_directory);

    for (int i = 0; i < profiles->len; ++i) {
        gchar *profile_name = g_ptr_array_index(profiles, i);

        GtkWidget *button = gtk_button_new_with_label(profile_name);
        gtk_widget_set_focus_on_click(button, false);
        gtk_widget_set_size_request(button, 0, 50); //TODO: refactor ?

        GtkWidget *label = gtk_bin_get_child(GTK_BIN(button));
        gtk_label_set_max_width_chars(GTK_LABEL(label), 0); //TODO: refactor ?
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);

        gtk_box_pack_start(login_box, button, 0, 1, 0);
        gtk_widget_show(button);

        g_signal_connect(button, "clicked", G_CALLBACK(login_callback), profile_name);
    }

    g_ptr_array_free(profiles, TRUE);
}

LupusProfileChooser *lupus_profile_chooser_new(LupusApplication *application) {
    return g_object_new(LUPUS_TYPE_PROFILE_CHOOSER,
                        "application", application,
                        NULL);
}

static void set_password_callback(LupusProfileChooserPasswordDialog *password_dialog, gchar *data, gchar **password) {
    *password = g_strdup(data);
}