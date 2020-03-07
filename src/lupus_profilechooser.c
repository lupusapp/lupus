#include "../include/lupus_profilechooser.h"
#include "../include/lupus.h"
#include "../include/lupus_main.h"
#include "../include/utils.h"
#include "toxcore/tox.h"
#include "toxencryptsave/toxencryptsave.h"
#include <errno.h>

#define DEFAULT_NAME "Lupus's user"
#define DEFAULT_STATUS_MESSAGE "Lupus's rocks !"
#define PROFILE_HEIGHT 50
#define PASSWORD_DIALOG_WIDTH 250
#define PASSWORD_DIALOG_MARGIN 20

struct _LupusProfileChooser {
    GtkApplicationWindow parent_instance;

    GtkStack *stack;
    GtkBox *login_box;
    GtkEntry *register_name, *register_pass;
    GtkButton *register_button;
};

G_DEFINE_TYPE(LupusProfileChooser, lupus_profilechooser,
              GTK_TYPE_APPLICATION_WINDOW)

static gchar *ask_password(void) {
    GtkWidget *dialog =
        g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Decrypt", GTK_RESPONSE_ACCEPT);
    gtk_widget_set_size_request(dialog, PASSWORD_DIALOG_WIDTH, 0);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_set_margin_top(entry, PASSWORD_DIALOG_MARGIN);
    gtk_widget_set_margin_end(entry, PASSWORD_DIALOG_MARGIN);
    gtk_widget_set_margin_bottom(entry, PASSWORD_DIALOG_MARGIN);
    gtk_widget_set_margin_start(entry, PASSWORD_DIALOG_MARGIN);

    gtk_container_add(gtk_container_get_children(GTK_CONTAINER(dialog))->data,
                      entry);

    gtk_widget_show_all(dialog);

    gchar *password = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_DELETE_EVENT) {
        goto destroy;
    }
    password = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

destroy:
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return password;
}

static void login_cb(GtkButton *button, LupusProfileChooser *instance) {
    gchar *filename =
        g_strconcat(LUPUS_TOX_DIR, gtk_button_get_label(button), ".tox", NULL);

    GError *error = NULL;
    gchar *savedata = NULL;
    gsize savedata_size = 0;
    g_file_get_contents(filename, &savedata, &savedata_size, &error);
    if (error) {
        lupus_error(instance, "Cannot open profiles: \"%s\".", error->message);
        g_error_free(error);
        goto free;
    }

    gchar *password = NULL;
    if (tox_is_data_encrypted((guint8 *)savedata)) {
        if (!(password = ask_password())) {
            goto free;
        }

        guint8 *tmp =
            g_malloc(savedata_size - TOX_PASS_ENCRYPTION_EXTRA_LENGTH);
        if (!tox_pass_decrypt((guint8 *)savedata, savedata_size,
                              (guint8 *)password, strlen(password), tmp,
                              NULL)) {
            lupus_error(instance, "Cannot decrypt profile.");
            g_free(tmp);
            goto free;
        }

        g_free(savedata);
        savedata = (gchar *)tmp;
        savedata_size -= TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    struct Tox_Options *options = tox_options_new(NULL);
    if (!options) {
        lupus_error(instance, "Cannot create <i>Tox_Options</i>.");
        goto free;
    }
    tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);
    tox_options_set_savedata_data(options, (guint8 *)savedata, savedata_size);

    struct Tox *tox = tox_new(options, NULL);
    if (!tox) {
        lupus_error(instance, "Cannot create <i>Tox</i>.");
        tox_options_free(options);
        goto free;
    }

    gtk_window_present(GTK_WINDOW(
        lupus_main_new(gtk_window_get_application(GTK_WINDOW(instance)), tox,
                       g_strdup(filename), g_strdup(password))));
    gtk_widget_destroy(GTK_WIDGET(instance));

free:
    g_free(password); // NOLINT
    g_free(filename);
}

static void register_cb(LupusProfileChooser *instance) {
    gchar *filename =
        g_strconcat(LUPUS_TOX_DIR, gtk_entry_get_text(instance->register_name),
                    ".tox", NULL);

    if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
        lupus_error(instance, "Profile \"%s\" already exists.", filename);
        goto free;
    }

    Tox *tox = tox_new(NULL, NULL);
    if (!tox) {
        lupus_error(instance, "Cannot create profile.");
        goto free;
    }

    // not a problem if name or status fail
    tox_self_set_name(tox, (guint8 *)DEFAULT_NAME, strlen(DEFAULT_NAME), NULL);
    tox_self_set_status_message(tox, (guint8 *)DEFAULT_STATUS_MESSAGE,
                                strlen(DEFAULT_STATUS_MESSAGE), NULL);

    tox_save(tox, filename, gtk_entry_get_text(instance->register_pass),
             GTK_WINDOW(instance), true);
    tox_kill(tox);

free:
    g_free(filename);
}

static void list_profiles(LupusProfileChooser *instance) {
    GError *error = NULL;
    GDir *dir = g_dir_open(LUPUS_TOX_DIR, 0, &error);
    if (error) {
        lupus_error(instance, "Cannot list profiles: %s", error->message);
        g_error_free(error);
        return;
    }

    GPtrArray *profiles = g_ptr_array_new();
    gchar const *file = NULL;
    while ((file = g_dir_read_name(dir))) {
        if (g_str_has_suffix(file, ".tox")) {
            g_ptr_array_add(profiles, g_strdup(file));
        }
    }
    g_dir_close(dir);

    if (errno) {
        lupus_error(instance, "Cannot get all files: %s", g_strerror(errno));
        errno = 0;
    }

    gtk_container_foreach(GTK_CONTAINER(instance->login_box),
                          (GtkCallback)gtk_widget_destroy, NULL);

    for (guint i = 0; i < profiles->len; ++i) {
        gchar *name =
            g_strndup(profiles->pdata[i], strlen(profiles->pdata[i]) - 4);

        GtkWidget *button = gtk_button_new_with_label(name);
        gtk_widget_set_focus_on_click(button, FALSE);
        gtk_widget_set_size_request(button, 0, PROFILE_HEIGHT);
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

        GtkWidget *label = gtk_bin_get_child(GTK_BIN(button));
        gtk_label_set_max_width_chars(GTK_LABEL(label), 0);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
        gtk_widget_set_tooltip_text(label, name);

        g_free(name);

        gtk_box_pack_start(instance->login_box, button, FALSE, TRUE, 0);
        gtk_widget_show(button);

        g_signal_connect(button, "clicked", G_CALLBACK(login_cb), instance);
    }

    g_ptr_array_free(profiles, TRUE);
}

static void stack_change_cb(LupusProfileChooser *instance) {
    if (g_strcmp0(gtk_stack_get_visible_child_name(instance->stack), "login") ==
        0) {
        list_profiles(instance);
    }
}

static void lupus_profilechooser_class_init(LupusProfileChooserClass *class) {
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(class), LUPUS_RESOURCES "/profilechooser.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, stack);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusProfileChooser, login_box);
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
    g_signal_connect_swapped(instance->stack, "notify::visible-child-name",
                             G_CALLBACK(stack_change_cb), instance);

    list_profiles(instance);

    gtk_widget_show_all(GTK_WIDGET(instance));
}

LupusProfileChooser *lupus_profilechooser_new(LupusApplication *application) {
    return g_object_new(LUPUS_TYPE_PROFILECHOOSER, "application", application,
                        NULL);
}