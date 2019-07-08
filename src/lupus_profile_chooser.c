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
    LupusProfileChooserPrivate *private;

    gtk_widget_init_template(GTK_WIDGET(instance));

    private = lupus_profile_chooser_get_instance_private(instance);
    gtk_header_bar_set_subtitle(private->header_bar, LUPUS_VERSION);

    g_signal_connect(private->stack, "notify::visible-child", G_CALLBACK(stack_change_callback), instance);
    g_signal_connect(private->register_button, "clicked", G_CALLBACK(register_callback), instance);

    list_tox_profile(private->login_box);
}

static void lupus_profile_chooser_class_init(LupusProfileChooserClass *class) {
    gchar *resource;

    resource = g_strconcat(LUPUS_RESOURCES, "/profile_chooser.ui", NULL);
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
    gchar const *name;
    LupusProfileChooserPrivate *priv;
    gboolean is_register;
    GtkContainer *container;

    name = gtk_stack_get_visible_child_name(GTK_STACK(gobject));
    priv = lupus_profile_chooser_get_instance_private(LUPUS_PROFILE_CHOOSER(user_data));
    is_register = g_strcmp0(name, "register") == 0;
    container = GTK_CONTAINER(is_register ? priv->login_box : priv->register_box);

    if (is_register == false) {
        list_tox_profile(priv->login_box);
    }

    gtk_widget_show_all(GTK_WIDGET(user_data));
    gtk_container_foreach(container, (gpointer) gtk_widget_hide, NULL);
}

static void register_callback(GtkButton *button, gpointer user_data) {
    TOX_ERR_NEW tox_err_new;
    Tox *tox;
    TOX_ERR_SET_INFO tox_err_set_info;
    char *user, *status_message;
    size_t tox_data_size, tox_data_encrypted_size;
    uint8_t *tox_data, *tox_data_encrypted;
    LupusProfileChooserPrivate *priv;
    gchar const *profile_name, *profile_password, *profile_filename;
    GError *error;
    TOX_ERR_ENCRYPTION tox_err_encryption;

    tox = tox_new(NULL, &tox_err_new);
    g_assert(tox_err_new == TOX_ERR_NEW_OK);

    user = "Lupus's user";
    tox_self_set_name(tox, (uint8_t *) user, strlen(user), &tox_err_set_info);
    g_assert(tox_err_set_info == TOX_ERR_SET_INFO_OK);

    status_message = "Lupus rocks !";
    tox_self_set_status_message(tox, (uint8_t *) status_message, strlen(status_message), &tox_err_set_info);
    g_assert(tox_err_set_info == TOX_ERR_SET_INFO_OK);

    tox_data_size = tox_get_savedata_size(tox);
    tox_data = g_malloc(tox_data_size);
    tox_get_savedata(tox, tox_data);

    priv = lupus_profile_chooser_get_instance_private(LUPUS_PROFILE_CHOOSER(user_data));
    profile_name = gtk_entry_get_text(priv->register_name);
    profile_password = gtk_entry_get_text(priv->register_password); //TODO

    profile_filename = g_strconcat(LUPUS_TOX_DIR, profile_name, ".tox", NULL);
    g_assert(g_file_test(profile_filename, G_FILE_TEST_EXISTS) == false);

    error = NULL;
    if (strlen(profile_password) != 0) {
        tox_data_encrypted_size = tox_data_size + TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
        tox_data_encrypted = g_malloc(tox_data_encrypted_size);

        tox_pass_encrypt(
                tox_data, tox_data_size,
                (uint8_t *) profile_password, strlen(profile_password),
                tox_data_encrypted,
                &tox_err_encryption
        );

        g_assert(tox_err_encryption == TOX_ERR_ENCRYPTION_OK);
        g_assert(tox_is_data_encrypted(tox_data_encrypted));

        printf("tox encrypted: %lu\n", tox_data_encrypted_size);

        g_file_set_contents(profile_filename, (gchar *) tox_data_encrypted, tox_data_encrypted_size, &error);
        g_free(tox_data_encrypted);
    } else {
        g_file_set_contents(profile_filename, (gchar *) tox_data, tox_data_size, &error);
        g_free(tox_data);
    }
    g_assert(error == NULL);

    g_free((gpointer) profile_filename);
    if (error != NULL) {
        g_error_free(error);
    }
}

static void login_callback(GtkButton *button, gpointer user_data) {
    gchar *filename, *content, *password;
    GError *error;
    gsize content_length, tox_decrypted_length;
    TOX_ERR_OPTIONS_NEW tox_err_options_new;
    struct Tox_Options *options;
    TOX_ERR_NEW tox_err_new;
    Tox *tox;
    LupusProfileChooserPasswordDialog *password_dialog;
    uint8_t *tox_decrypted;
    TOX_ERR_DECRYPTION tox_err_decryption;

    /* TODO: check if directory exists */

    filename = g_strconcat(LUPUS_TOX_DIR, (gchar *) user_data, NULL);
    g_assert(g_file_test(filename, G_FILE_TEST_EXISTS));

    error = NULL;
    content = NULL;
    g_file_get_contents(filename, &content, &content_length, &error);
    g_assert(error == NULL);
    g_free(filename);
    if (error != NULL) {
        g_error_free(error);
    }

    options = tox_options_new(&tox_err_options_new);
    g_assert(tox_err_options_new == TOX_ERR_OPTIONS_NEW_OK);
    tox_decrypted = NULL;

    tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);

    if (tox_is_data_encrypted((uint8_t *) content)) {
        password = NULL;
        password_dialog = lupus_profile_chooser_password_dialog_new();
        g_signal_connect(password_dialog, "decrypt", G_CALLBACK(set_password_callback), &password);

        if (gtk_dialog_run(GTK_DIALOG(password_dialog)) != GTK_RESPONSE_ACCEPT) {
            gtk_widget_destroy(GTK_WIDGET(password_dialog));
            goto free;
        }

        gtk_widget_destroy(GTK_WIDGET(password_dialog));

        tox_decrypted_length = content_length - TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
        tox_decrypted = g_malloc(tox_decrypted_length);

        tox_pass_decrypt(
                (uint8_t *) content, content_length,
                (uint8_t *) password, strlen(password),
                tox_decrypted,
                &tox_err_decryption
        );
        g_assert(tox_err_decryption == TOX_ERR_DECRYPTION_OK);

        tox_options_set_savedata_data(options, (uint8_t *) tox_decrypted, tox_decrypted_length);

        g_free(password);
    } else {
        tox_options_set_savedata_data(options, (uint8_t *) content, content_length);
    }

    tox = tox_new(options, &tox_err_new);
    g_assert(tox_err_new == TOX_ERR_NEW_OK);

free:
    tox_options_free(options);
    g_free(content);
    if (tox_decrypted != NULL) {
        g_free(tox_decrypted);
    }
}

void list_tox_profile(GtkBox *login_box) {
    GError *error;
    GDir *tox_dir;
    GPtrArray *profiles;
    GtkWidget *button, *label;
    gchar *filename, *profile_name;
    guint i;

    gtk_container_foreach(GTK_CONTAINER(login_box), (gpointer) gtk_widget_destroy, NULL);

    error = NULL;
    tox_dir = g_dir_open(LUPUS_TOX_DIR, 0, &error);
    g_assert(error == NULL);
    if (error != NULL) {
        g_error_free(error);
    }

    profiles = g_ptr_array_new();
    while ((filename = (gchar *) g_dir_read_name(tox_dir))) {
        if (g_str_has_suffix(filename, ".tox")) {
            g_ptr_array_add(profiles, filename);
        }
    }

    for (i = 0; i < profiles->len; ++i) {
        profile_name = g_ptr_array_index(profiles, i);

        button = gtk_button_new_with_label(profile_name);
        gtk_widget_set_focus_on_click(button, false);
        gtk_widget_set_size_request(button, 0, 50); //TODO: refactor ?

        label = gtk_bin_get_child(GTK_BIN(button));
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
    *password = g_malloc(strlen(data) + 1);
    strcpy(*password, data);
}