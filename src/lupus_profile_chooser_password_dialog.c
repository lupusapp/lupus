#include "../include/lupus_profile_chooser_password_dialog.h"
#include "../include/lupus.h"

struct _LupusProfileChooserPasswordDialog {
    GtkDialog parent_instance;
};

typedef struct _LupusProfileChooserPasswordDialogPrivate LupusProfileChooserPasswordDialogPrivate;
struct _LupusProfileChooserPasswordDialogPrivate {
    GtkButton *decrypt;
    GtkEntry *password;
};

enum {
    DECRYPT,

    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE(LupusProfileChooserPasswordDialog, lupus_profile_chooser_password_dialog, GTK_TYPE_DIALOG)

static void lupus_profile_chooser_password_dialog_init(LupusProfileChooserPasswordDialog *instance) {
    LupusProfileChooserPasswordDialogPrivate *priv;

    gtk_widget_init_template(GTK_WIDGET(instance));

    priv = lupus_profile_chooser_password_dialog_get_instance_private(instance);

    g_signal_connect(priv->decrypt, "clicked", G_CALLBACK(decrypt_callback), instance);
}

static void lupus_profile_chooser_password_dialog_class_init(LupusProfileChooserPasswordDialogClass *class) {
    gchar *resource;

    resource = g_strconcat(LUPUS_RESOURCES, "/profile_chooser_password_dialog.ui", NULL);
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), resource);
    g_free(resource);

    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooserPasswordDialog, decrypt);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfileChooserPasswordDialog, password);

    signals[DECRYPT] = g_signal_new(
            "decrypt",
            LUPUS_TYPE_PROFILE_CHOOSER_PASSWORD_DIALOG,
            G_SIGNAL_RUN_LAST,
            0, NULL, NULL, NULL,
            G_TYPE_NONE, 1,
            G_TYPE_STRING
    );
}

static void decrypt_callback(GtkButton *button, gpointer user_data) {
    LupusProfileChooserPasswordDialogPrivate *priv;
    gchar const *password;

    priv = lupus_profile_chooser_password_dialog_get_instance_private(LUPUS_PROFILE_CHOOSER_PASSWORD_DIALOG(user_data));
    password = gtk_entry_get_text(GTK_ENTRY(priv->password));

    g_signal_emit(user_data, signals[DECRYPT], 0, password);
    g_signal_emit_by_name(user_data, "response", GTK_RESPONSE_ACCEPT);
}

LupusProfileChooserPasswordDialog *lupus_profile_chooser_password_dialog_new(void) {
    return g_object_new(LUPUS_TYPE_PROFILE_CHOOSER_PASSWORD_DIALOG,
                        "use-header-bar", TRUE,
                        NULL);
}