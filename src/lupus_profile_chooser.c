#include "../include/lupus_profile_chooser.h"
#include <errno.h>
#include "toxcore/tox.h"
#include "toxencryptsave/toxencryptsave.h"
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

    list_tox_profile(private->login_box, instance);
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
    LupusProfileChooserPrivate *priv = lupus_profile_chooser_get_instance_private(LUPUS_PROFILE_CHOOSER(user_data));
    gboolean is_register = g_strcmp0(gtk_stack_get_visible_child_name(GTK_STACK(gobject)), "register") == 0;

    if (is_register == false) {
        list_tox_profile(priv->login_box, LUPUS_PROFILE_CHOOSER(user_data));
    }

    gtk_widget_show_all(GTK_WIDGET(user_data));
    gtk_container_foreach(GTK_CONTAINER(is_register ? priv->login_box : priv->register_box),
                          (gpointer) gtk_widget_hide,
                          NULL);
}

static void register_callback(GtkButton *button, gpointer user_data) {
    LupusProfileChooserPrivate *priv = lupus_profile_chooser_get_instance_private(LUPUS_PROFILE_CHOOSER(user_data));
    char const *profile_name = gtk_entry_get_text(priv->register_name);
    char const *profile_password = gtk_entry_get_text(priv->register_password);

    char *tox_directory = LUPUS_TOX_DIR;
    if (!g_file_test(tox_directory, G_FILE_TEST_IS_DIR)) {
        error_message(user_data, "<b>Error</b>: directory %s does not exists.", tox_directory);
        g_free(tox_directory);
        return;
    }

    char *profile_filename = g_strconcat(tox_directory, profile_name, ".tox", NULL);
    if (g_file_test(profile_filename, G_FILE_TEST_EXISTS)) {
        error_message(user_data, "<b>Error</b>: Profile %s already exists.", profile_filename);
        g_free(tox_directory);
        g_free(profile_filename);
        return;
    }
    g_free(tox_directory);
    g_free(profile_filename);

    TOX_ERR_NEW tox_err_new;
    Tox *tox = tox_new(NULL, &tox_err_new);
    if (tox_err_new != TOX_ERR_NEW_OK) {
        error_message(user_data, "<b>Error</b>: tox_new returned %d.", tox_err_new);
        return;
    }

    TOX_ERR_SET_INFO tox_err_set_info;

    char *user = "Lupus's user";
    tox_self_set_name(tox, (uint8_t *) user, strlen(user), &tox_err_set_info);
    if (tox_err_set_info != TOX_ERR_SET_INFO_OK) {
        error_message(user_data, "<b>Error</b>: tox_self_set_name returned %d.", tox_err_set_info);
        tox_kill(tox);
        return;
    }

    char *status_message = "Lupus rocks !";
    tox_self_set_status_message(tox, (uint8_t *) status_message, strlen(status_message), &tox_err_set_info);
    if (tox_err_set_info != TOX_ERR_SET_INFO_OK) {
        error_message(user_data, "<b>Error</b>: tox_self_set_status_message returned %d.", tox_err_set_info);
        tox_kill(tox);
        return;
    };

    save_tox(tox, profile_name, profile_password, user_data);

    tox_kill(tox);
}

static void login_callback(GtkButton *button, gpointer user_data) {
    gchar *tox_directory = LUPUS_TOX_DIR;
    if (!g_file_test(tox_directory, G_FILE_TEST_IS_DIR)) {
        error_message(user_data, "<b>Error</b>: directory %s does not exists.", tox_directory);
        g_free(tox_directory);
        return;
    }

    gchar *filename = g_strconcat(tox_directory, gtk_button_get_label(button), NULL);
    if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
        error_message(user_data, "<b>Error</b>: file %s does not exists.", filename);
        g_free(tox_directory);
        g_free(filename);
        return;
    }

    g_free(tox_directory);

    GError *error = NULL;
    gchar *content = NULL;
    gsize content_length;
    g_file_get_contents(filename, &content, &content_length, &error);
    if (error != NULL) {
        error_message(user_data, "<b>Error</b>: %d<br>%s.", error->code, error->message);
        g_free(filename);
        g_free(content);
        g_error_free(error);
        return;
    }

    gchar *password = NULL;
    if (tox_is_data_encrypted((uint8_t *) content)) {
        LupusProfileChooserPasswordDialog *password_dialog = lupus_profile_chooser_password_dialog_new();

        g_signal_connect(password_dialog, "decrypt", G_CALLBACK(set_password_callback), &password);

        if (gtk_dialog_run(GTK_DIALOG(password_dialog)) != GTK_RESPONSE_ACCEPT) {
            gtk_widget_destroy(GTK_WIDGET(password_dialog));
            g_free(filename);
            g_free(content);
            return;
        }
        gtk_widget_destroy(GTK_WIDGET(password_dialog));

        TOX_ERR_DECRYPTION tox_err_decryption;
        uint8_t *content_decrypted = g_malloc(content_length - TOX_PASS_ENCRYPTION_EXTRA_LENGTH);
        tox_pass_decrypt(
                (uint8_t *) content, content_length,
                (uint8_t *) password, strlen(password),
                content_decrypted,
                &tox_err_decryption);
        if (tox_err_decryption != TOX_ERR_DECRYPTION_OK) {
            error_message(user_data, "<b>Error</b>: tox_pass_decrypted returned %d.", tox_err_decryption);
            g_free(filename);
            g_free(content);
            g_free(content_decrypted);
            return;
        }

        g_free(content);
        content = (char *) content_decrypted;
        content_length -= TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    TOX_ERR_OPTIONS_NEW tox_err_options_new;
    struct Tox_Options *options = tox_options_new(&tox_err_options_new);
    if (tox_err_options_new != TOX_ERR_OPTIONS_NEW_OK) {
        error_message(user_data, "<b>Error</b>: tox_options_new returned %d.", tox_err_options_new);
        g_free(filename);
        g_free(content);
        return;
    }
    tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);
    tox_options_set_savedata_data(options, (uint8_t *) content, content_length);

    TOX_ERR_NEW tox_err_new;
    Tox *tox = tox_new(options, &tox_err_new);
    if (tox_err_new != TOX_ERR_NEW_OK) {
        error_message(user_data, "<b>Error</b>: tox_new returned %d.", tox_err_new);
        g_free(filename);
        g_free(content);
        tox_options_free(options);
        return;
    }
    g_free(filename);
    g_free(content);
    tox_options_free(options);

    LupusMain *main = lupus_main_new(
            gtk_window_get_application(GTK_WINDOW(user_data)),
            tox,
            (gchar *) gtk_button_get_label(button),
            g_strdup(password));
    g_free(password);
    gtk_window_present(GTK_WINDOW(main));
    gtk_widget_destroy(GTK_WIDGET(user_data));
}

static void set_password_callback(LupusProfileChooserPasswordDialog *password_dialog, gchar *data, gchar **password) {
    *password = g_strdup(data);
}

void list_tox_profile(GtkBox *login_box, gpointer user_data) {
    gtk_container_foreach(GTK_CONTAINER(login_box), (gpointer) gtk_widget_destroy, NULL);

    gchar *tox_directory = LUPUS_TOX_DIR;
    if (!g_file_test(tox_directory, G_FILE_TEST_IS_DIR)) {
        error_message(user_data, "<b>Error</b>: directory %s does not exists.", tox_directory);
        g_free(tox_directory);
        return;
    }

    GError *error = NULL;
    GDir *tox_files = g_dir_open(tox_directory, 0, &error);
    if (error != NULL) {
        error_message(user_data, "<b>Error</b>: %d<br>%s.", error->code, error->message);
        g_free(tox_directory);
        g_error_free(error);
        if (tox_files != NULL) {
            g_dir_close(tox_files);
        }
        return;
    }
    g_free(tox_directory);

    GPtrArray *profiles = g_ptr_array_new();
    gchar *filename;
    while ((filename = (gchar *) g_dir_read_name(tox_files))) {
        if (errno != 0) {
            error_message(user_data, "<b>Error</b>: g_dir_read_name returned %s.", strerror(errno));
            continue;
        }

        if (g_str_has_suffix(filename, ".tox")) {
            g_ptr_array_add(profiles, g_strdup(filename));
        }
    }
    g_dir_close(tox_files);

    g_ptr_array_foreach(profiles, (GFunc) propagate_profiles, user_data);
    g_ptr_array_free(profiles, TRUE);
}

void propagate_profiles(gchar *data, gpointer user_data) {
    GtkWidget *button = gtk_button_new_with_label(data);
    gtk_widget_set_focus_on_click(button, false);
    gtk_widget_set_size_request(button, 0, 50);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

    GtkWidget *label = gtk_bin_get_child(GTK_BIN(button));
    gtk_label_set_max_width_chars(GTK_LABEL(label), 0);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);

    LupusProfileChooserPrivate *priv = lupus_profile_chooser_get_instance_private(LUPUS_PROFILE_CHOOSER(user_data));
    gtk_box_pack_start(priv->login_box, button, 0, 1, 0);
    gtk_widget_show(button);

    g_signal_connect(button, "clicked", G_CALLBACK(login_callback), user_data);
}

LupusProfileChooser *lupus_profile_chooser_new(LupusApplication *application) {
    return g_object_new(LUPUS_TYPE_PROFILE_CHOOSER,
                        "application", application,
                        NULL);
}