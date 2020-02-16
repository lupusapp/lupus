#include "../include/lupus_editablelabel.h"
#include "../include/lupus.h"

struct _LupusEditableLabel {
    GtkEventBox parent_instance;

    char const *value;

    GtkLabel *label;
    GtkPopover *popover;
    GtkEntry *entry;
    GtkButton *submit;
};

G_DEFINE_TYPE(LupusEditableLabel, lupus_editablelabel, GTK_TYPE_EVENT_BOX)

enum { PROP_VALUE = 1, N_PROPERTIES };

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

enum { SUBMIT, LAST_SIGNAL };

static guint signals[LAST_SIGNAL];

static void click_cb(LupusEditableLabel *instance) {
    gtk_popover_popup(instance->popover);
}

static void submit_cb(LupusEditableLabel *instance) {
    if (g_strcmp0(gtk_label_get_text(instance->label),
                  gtk_entry_get_text(instance->entry))) {
        g_signal_emit(instance, signals[SUBMIT], 0,
                      gtk_entry_get_text(instance->entry));
    }

    gtk_popover_popdown(instance->popover);
}

static void lupus_editablelabel_set_property(LupusEditableLabel *instance,
                                             guint property_id,
                                             GValue const *value,
                                             GParamSpec *pspec) {
    if (property_id != PROP_VALUE) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
        return;
    }

    instance->value = g_value_dup_string(value);
    gtk_label_set_text(instance->label, instance->value);
    gtk_entry_set_text(instance->entry, instance->value);

    g_free((gchar *)g_value_get_string(value));
}

static void lupus_editablelabel_get_property(LupusEditableLabel *instance,
                                             guint property_id, GValue *value,
                                             GParamSpec *pspec) {
    if (property_id != PROP_VALUE) {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
        return;
    }

    g_value_set_string(value, instance->value);
}

static void lupus_editablelabel_dispose(LupusEditableLabel *instance) {
    g_free((gchar *)instance->value);

    G_OBJECT_CLASS(lupus_editablelabel_parent_class) // NOLINT
        ->dispose(G_OBJECT(instance));               // NOLINT
}

static void lupus_editablelabel_class_init(LupusEditableLabelClass *class) {
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(class), LUPUS_RESOURCES "/editablelabel.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusEditableLabel, label);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusEditableLabel, popover);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusEditableLabel, entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusEditableLabel, submit);

    G_OBJECT_CLASS(class)->set_property = // NOLINT
        lupus_editablelabel_set_property;
    G_OBJECT_CLASS(class)->get_property = // NOLINT
        lupus_editablelabel_get_property;
    G_OBJECT_CLASS(class)->dispose = // NOLINT
        lupus_editablelabel_dispose;

    obj_properties[PROP_VALUE] =
        g_param_spec_string("value", "Value", "Value of the editable label.",
                            "", G_PARAM_READWRITE);

    g_object_class_install_properties(G_OBJECT_CLASS(class), // NOLINT
                                      N_PROPERTIES, obj_properties);

    signals[SUBMIT] =
        g_signal_new("submit", LUPUS_TYPE_EDITABLELABEL, G_SIGNAL_RUN_LAST, 0,
                     NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING); // NOLINT
}

static void lupus_editablelabel_init(LupusEditableLabel *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    gtk_button_set_image(
        instance->submit,
        gtk_image_new_from_icon_name("gtk-apply", GTK_ICON_SIZE_BUTTON));

    g_signal_connect_swapped(instance, "button-release-event",
                             G_CALLBACK(click_cb), instance);
    g_signal_connect_swapped(instance->submit, "clicked", G_CALLBACK(submit_cb),
                             instance);
}

LupusEditableLabel *lupus_editablelabel_new(gchar const *value) {
    return g_object_new(LUPUS_TYPE_EDITABLELABEL, "value", value, NULL);
}