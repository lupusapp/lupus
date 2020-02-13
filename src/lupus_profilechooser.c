#include "../include/lupus_profilechooser.h"
#include "../include/lupus.h"
#include "../include/lupus_profilechooserpassworddialog.h"
#include "../include/utils.h"
#include "toxcore/tox.h"
#include "toxencryptsave/toxencryptsave.h"
#include <sodium.h>

#define DEFAULT_NAME "Lupus's user"
#define DEFAULT_STATUS_MESSAGE "Lupus's rocks !"
#define PROFILE_HEIGHT 50

struct _LupusProfileChooser {
    GtkApplicationWindow parent_instance;

    GtkHeaderBar *header_bar;
    GtkStack *stack;
    GtkBox *login_box, *register_box;
    GtkEntry *register_name, *register_pass;
    GtkButton *register_button;
};

G_DEFINE_TYPE(LupusProfileChooser, lupus_profilechooser,
              GTK_TYPE_APPLICATION_WINDOW)

/*
 * data = {
 *  "profile filename",
 *  "profile password"
 * }
 */
static void tox_save(Tox *tox, char const **data,
                     LupusProfileChooser *instance) {
    size_t savedata_size = tox_get_savedata_size(tox);
    uint8_t *savedata = g_malloc(savedata_size);
    tox_get_savedata(tox, savedata);

    if (data[1] && *data[1]) {
        uint8_t *savedata_encrypted =
            g_malloc(savedata_size + TOX_PASS_ENCRYPTION_EXTRA_LENGTH);

        if (!tox_pass_encrypt(savedata, savedata_size, (uint8_t *)data[1],
                              strlen(data[1]), savedata_encrypted, NULL)) {
            lupus_error(instance, "Cannot create encrypted profile.");
            g_free(savedata_encrypted);
            goto free;
        }

        g_free(savedata);
        savedata = savedata_encrypted;
        savedata_size += TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    GError *error = NULL;
    g_file_set_contents(data[0], (char *)savedata, savedata_size, &error);
    if (error) {
        lupus_error(instance, "Cannot save profile: %s", error->message);
        g_error_free(error);
        goto free;
    }

    lupus_success(instance, "Profile \"%s\" created.", *data);

free:
    g_free(savedata);
}

// NOLINTNEXTLINE
static void strcpy_cb(LupusProfileChooserPasswordDialog *dialog, char *source,
                      char **destination) {
    *destination = g_strdup(source);
}

static void login_cb(GtkButton *button, LupusProfileChooser *instance) {
    char const *filename =
        g_strconcat(LUPUS_TOX_DIR, gtk_button_get_label(button), ".tox", NULL);

    GError *error = NULL;
    char *savedata = NULL;
    gsize savedata_size;
    g_file_get_contents(filename, &savedata, &savedata_size, &error);
    if (error) {
        lupus_error(instance, "Cannot open profile: %s", error->message);
        goto free;
    }

    if (tox_is_data_encrypted((uint8_t *)savedata)) {
        LupusProfileChooserPasswordDialog *dialog =
            lupus_profilechooserpassworddialog_new();

        char *password = NULL;
        g_signal_connect(dialog, "submit", G_CALLBACK(strcpy_cb), &password);

        bool accept = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
        gtk_widget_destroy(GTK_WIDGET(dialog));
        if (!accept) {
            goto free;
        }

        uint8_t *savedata_decrypted =
            g_malloc(savedata_size - TOX_PASS_ENCRYPTION_EXTRA_LENGTH);
        if (!tox_pass_decrypt((uint8_t *)savedata, savedata_size,
                              (uint8_t *)password, strlen(password),
                              savedata_decrypted, NULL)) {
            lupus_error(instance, "Cannot decrypt profile.");
            g_free(savedata_decrypted);
            g_free(password);
            goto free;
        }

        g_free(password);
        g_free(savedata);
        savedata = (char *)savedata_decrypted;
        savedata_size -= TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    struct Tox_Options *options = tox_options_new(NULL);
    if (!options) {
        lupus_error(instance, "Cannot create <i>Tox_Options</i>.");
        goto free;
    }

    tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);
    tox_options_set_savedata_data(options, (uint8_t *)savedata, savedata_size);

    struct Tox *tox = tox_new(options, NULL);
    if (!tox) {
        lupus_error(instance, "Cannot create <i>Tox</i>.");
        goto free_a;
    }

    uint8_t address[TOX_ADDRESS_SIZE];
    tox_self_get_address(tox, address);

    char *hex[TOX_ADDRESS_SIZE * 2 + 1];

    lupus_success(instance, "ID: <i>%s</i>.",
                  sodium_bin2hex((char *)hex, TOX_ADDRESS_SIZE * 2 + 1, address,
                                 TOX_ADDRESS_SIZE));

free_a:
    g_free(tox);
    tox_options_free(options);
free:
    g_free((gpointer)filename);
    if (error) {
        g_error_free(error);
    }
    g_free(savedata);
}

static void register_cb(LupusProfileChooser *instance) {
    char const *data[2] = {
        g_strconcat(LUPUS_TOX_DIR, gtk_entry_get_text(instance->register_name),
                    ".tox", NULL),
        gtk_entry_get_text(instance->register_pass)};
    if (g_file_test(*data, G_FILE_TEST_EXISTS)) {
        lupus_error(instance, "Profile \"%s\" already exists.", *data);
        goto free;
    }

    Tox *tox = tox_new(NULL, NULL);
    if (!tox) {
        lupus_error(instance, "Cannot create profile.");
        goto free;
    }

    // not a problem if name or status fail
    tox_self_set_name(tox, (uint8_t *)DEFAULT_NAME, strlen(DEFAULT_NAME), NULL);
    tox_self_set_status_message(tox, (uint8_t *)DEFAULT_STATUS_MESSAGE,
                                strlen(DEFAULT_STATUS_MESSAGE), NULL);

    tox_save(tox, data, instance);
    tox_kill(tox);

free:
    g_free((gpointer)*data);
}

static void propagate(char *profile, LupusProfileChooser *instance) {
    GtkWidget *button =
        gtk_button_new_with_label(g_strndup(profile, strlen(profile) - 4));
    g_free(profile);

    gtk_widget_set_focus_on_click(button, false);
    gtk_widget_set_size_request(button, 0, PROFILE_HEIGHT);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

    GtkWidget *label = gtk_bin_get_child(GTK_BIN(button));
    gtk_label_set_max_width_chars(GTK_LABEL(label), 0);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);

    gtk_box_pack_start(instance->login_box, button, 0, 1, 0);
    gtk_widget_show(button);

    g_signal_connect(button, "clicked", G_CALLBACK(login_cb), instance);
}

static void list_profiles(LupusProfileChooser *instance) {
    gtk_container_foreach(GTK_CONTAINER(instance->login_box),
                          (GtkCallback)gtk_widget_destroy, NULL);

    GError *error = NULL;
    GDir *dir = g_dir_open(LUPUS_TOX_DIR, 0, &error);
    if (error) {
        lupus_error(instance, "Cannot list profiles: %s", error->message);
        goto free;
    }

    GPtrArray *profiles = g_ptr_array_new();
    char *file;
    /*TODO(ogromny): possible source of errors (check errno?)*/
    while ((file = (char *)g_dir_read_name(dir))) {
        if (g_str_has_suffix(file, ".tox")) {
            g_ptr_array_add(profiles, g_strdup(file));
        }
    }

    g_ptr_array_foreach(profiles, (GFunc)propagate, instance);
    g_ptr_array_free(profiles, 1);

free:
    if (error) {
        g_error_free(error);
    }
    if (dir) {
        g_dir_close(dir);
    }
}

static void stack_change_cb(GtkStack *stack, GParamSpec *pspec,
                            LupusProfileChooser *instance) {
    char *visible_child_name;
    g_object_get(G_OBJECT(stack), g_param_spec_get_name(pspec), // NOLINT
                 &visible_child_name, NULL);

    if (g_strcmp0(visible_child_name, "login") == 0) {
        list_profiles(instance);
    }
}

static void lupus_profilechooser_class_init(LupusProfileChooserClass *class) {
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(class), LUPUS_RESOURCES "/profilechooser.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, header_bar);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, stack);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, login_box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, register_box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, register_name);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, register_pass);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, register_button);
}

static void lupus_profilechooser_init(LupusProfileChooser *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect_swapped(instance->register_button, "clicked",
                             G_CALLBACK(register_cb), instance);

    g_signal_connect(instance->stack, "notify::visible-child-name",
                     G_CALLBACK(stack_change_cb), instance);

    gtk_widget_show_all(GTK_WIDGET(instance));

    list_profiles(instance);
}

LupusProfileChooser *lupus_profilechooser_new(LupusApplication *application) {
    return g_object_new(LUPUS_TYPE_PROFILECHOOSER, "application", application,
                        NULL);
}