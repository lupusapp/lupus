#include "../include/lupus_editablelabel.h"

struct _LupusEditableLabel {
    GtkEventBox parent_instance;

    gchar *value;

    GtkLabel *label;
    GtkPopover *popover;
    GtkEntry *entry;
    GtkButton *submit;
};

G_DEFINE_TYPE(LupusEditableLabel, lupus_editablelabel, GTK_TYPE_EVENT_BOX)

#define MIN_LENGTH 1U << 0U
#define MAX_LENGTH 1U << 12U

typedef enum { PROP_VALUE = 1, PROP_MAX_LENGTH, N_PROPERTIES } LupusEditableLabelProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

typedef enum {
    SUBMIT,
    LAST_SIGNAL,
} LupusEditableLabelSignal;
static guint signals[LAST_SIGNAL];

setter(editablelabel, EditableLabel, value, char *);

static void clicked_cb(LupusEditableLabel *instance)
{
    gchar const *label = gtk_label_get_text(instance->label);
    gchar const *entry = gtk_entry_get_text(instance->entry);

    if (g_strcmp0(label, entry) != 0) {
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

static void lupus_editablelabel_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusEditableLabel *instance = LUPUS_EDITABLELABEL(object);

    switch (property_id) {
    case PROP_VALUE:
        instance->value = g_value_dup_string(value);
        gtk_label_set_text(instance->label, instance->value);
        gtk_entry_set_text(instance->entry, instance->value);
        break;
    case PROP_MAX_LENGTH:
        gtk_entry_set_max_length(instance->entry, g_value_get_uint(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_editablelabel_finalize(GObject *object)
{
    LupusEditableLabel *instance = LUPUS_EDITABLELABEL(object);

    g_free(instance->value);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_editablelabel_parent_class); // NOLINT
    object_class->finalize(object);
}

static void lupus_editablelabel_class_init(LupusEditableLabelClass *class)
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

    gint param = G_PARAM_WRITABLE;
    obj_properties[PROP_VALUE] = g_param_spec_string("value", "Value", "Label value", "", param);
    obj_properties[PROP_MAX_LENGTH] = g_param_spec_uint("max-length", "Max Length", "Label maximum length.", MIN_LENGTH,
                                                        MAX_LENGTH, MIN_LENGTH, param);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    signals[SUBMIT] = g_signal_new("submit", LUPUS_TYPE_EDITABLELABEL, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                   G_TYPE_BOOLEAN, 1, G_TYPE_STRING); // NOLINT
}

static void lupus_editablelabel_init(LupusEditableLabel *instance)
{
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect_swapped(instance, "button-release-event", G_CALLBACK(gtk_popover_popup), instance->popover);
    g_signal_connect_swapped(instance->submit, "clicked", G_CALLBACK(clicked_cb), instance);
}

LupusEditableLabel *lupus_editablelabel_new(gchar *value, guint max_length)
{
    return g_object_new(LUPUS_TYPE_EDITABLELABEL, "value", value, "max-length", max_length, NULL);
}