#include "../include/lupus_profilechooserpassworddialog.h"
#include "../include/lupus.h"

struct _LupusProfileChooserPasswordDialog {
    GtkDialog parent_instance;

    GtkButton *submit;
    GtkEntry *entry;
};

enum { SUBMIT, LAST_SIGNAL };

static unsigned signals[LAST_SIGNAL];

G_DEFINE_TYPE(LupusProfileChooserPasswordDialog,
              lupus_profilechooserpassworddialog, GTK_TYPE_DIALOG)

static void submit_cb(LupusProfileChooserPasswordDialog *instance) {
    g_signal_emit_by_name(instance, "response", GTK_RESPONSE_ACCEPT);
    g_signal_emit(instance, signals[SUBMIT], 0,
                  gtk_entry_get_text(instance->entry));
}

static void lupus_profilechooserpassworddialog_class_init(
    LupusProfileChooserPasswordDialogClass *class) {
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(class),
        LUPUS_RESOURCES "/profilechooserpassworddialog.ui");

    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(class), LupusProfileChooserPasswordDialog, submit);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(class), LupusProfileChooserPasswordDialog, entry);

    signals[SUBMIT] = g_signal_new(
        "submit", LUPUS_TYPE_PROFILECHOOSERPASSWORDDIALOG, G_SIGNAL_RUN_LAST, 0,
        NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING); // NOLINT
}

static void lupus_profilechooserpassworddialog_init(
    LupusProfileChooserPasswordDialog *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect_swapped(instance->submit, "clicked", G_CALLBACK(submit_cb),
                             instance);
}

LupusProfileChooserPasswordDialog *
lupus_profilechooserpassworddialog_new(void) {
    return g_object_new(LUPUS_TYPE_PROFILECHOOSERPASSWORDDIALOG,
                        "use-header-bar", 1, NULL);
}
