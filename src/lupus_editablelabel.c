#include "../include/lupus_editablelabel.h"
#include "include/lupus.h"

struct _LupusEditableLabel {
    GtkEventBox parent_instance;

    gchar *value;

    GtkLabel *label;
    GtkPopover *popover;
    GtkEntry *entry;
    GtkButton *submit;
};

G_DEFINE_TYPE(LupusEditableLabel, lupus_editablelabel, GTK_TYPE_EVENT_BOX)

#define t_n lupus_editablelabel
#define TN  LupusEditableLabel
#define T_N LUPUS_EDITABLELABEL

#define MIN_LENGTH 1U << 0U
#define MAX_LENGTH 1U << 12U

declare_properties(PROP_VALUE, PROP_MAX_LENGTH);
declare_signals(SUBMIT);

static void clicked_cb(INSTANCE)
{
    gchar const *label = gtk_label_get_text(instance->label);
    gchar const *entry = gtk_entry_get_text(instance->entry);

    if (g_strcmp0(label, entry)) {
        gboolean result = FALSE;
        g_signal_emit(instance, signals[SUBMIT], 0, entry, &result);

        if (result) {
            g_free(instance->value);
            instance->value = g_strdup(entry);
            gtk_label_set_text(instance->label, instance->value);
            gtk_entry_set_text(instance->entry, instance->value);
        }
    }

    gtk_popover_popdown(instance->popover);
}

set_property_header()
case PROP_VALUE:
    instance->value = g_value_dup_string(value);
    gtk_label_set_text(instance->label, instance->value);
    gtk_entry_set_text(instance->entry, instance->value);
    break;
case PROP_MAX_LENGTH:
    gtk_entry_set_max_length(instance->entry, g_value_get_uint(value));
    break;
set_property_footer()

finalize_header()
    g_free(instance->value);
finalize_footer()

class_init()
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass *object_class = G_OBJECT_CLASS(class); // NOLINT

    gtk_widget_class_set_template_from_resource(widget_class, LUPUS_RESOURCES "/editablelabel.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusEditableLabel, label);
    gtk_widget_class_bind_template_child(widget_class, LupusEditableLabel, popover);
    gtk_widget_class_bind_template_child(widget_class, LupusEditableLabel, entry);
    gtk_widget_class_bind_template_child(widget_class, LupusEditableLabel, submit);

    object_class->set_property = lupus_editablelabel_set_property;
    object_class->finalize = lupus_editablelabel_finalize;

    define_property(PROP_VALUE, string, "value", NULL, G_PARAM_WRITABLE);
    define_property(PROP_MAX_LENGTH, uint, "max-length", MIN_LENGTH, MAX_LENGTH, MIN_LENGTH, G_PARAM_WRITABLE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    define_signal(SUBMIT, "submit", LUPUS_TYPE_EDITABLELABEL, G_TYPE_BOOLEAN, 1, G_TYPE_STRING);
}

init()
{
    gtk_widget_init_template(GTK_WIDGET(instance));

    connect_swapped(instance, "button-release-event", gtk_popover_popup, instance->popover);
    connect_swapped(instance->submit, "clicked", clicked_cb, instance);
}

LupusEditableLabel *lupus_editablelabel_new(gchar *value, guint max_length)
{
    return g_object_new(LUPUS_TYPE_EDITABLELABEL, "value", value, "max-length", max_length, NULL);
}

