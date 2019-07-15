#include "../include/lupus_editable_label.h"

struct _LupusEditableLabel {
    GtkEventBox parent_instance;
};

typedef struct _LupusEditableLabelPrivate LupusEditableLabelPrivate;
struct _LupusEditableLabelPrivate {
    gchar *text;
    GtkWidget *label, *box, *entry, *button, *popover;
};

G_DEFINE_TYPE_WITH_PRIVATE(LupusEditableLabel, lupus_editable_label, GTK_TYPE_EVENT_BOX)

enum {
    EDITED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

enum {
    PROP_TEXT = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void
lupus_editable_label_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec) {
    LupusEditableLabelPrivate *priv = lupus_editable_label_get_instance_private(LUPUS_EDITABLE_LABEL(object));

    switch (property_id) {
        case PROP_TEXT:
            //TODO: handle reset
            priv->text = g_value_dup_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_editable_label_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
    LupusEditableLabelPrivate *priv = lupus_editable_label_get_instance_private(LUPUS_EDITABLE_LABEL(object));

    switch (property_id) {
        case PROP_TEXT:
            g_value_set_string(value, priv->text);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void confirm_callback(GtkButton *button, gpointer user_data) {
    LupusEditableLabelPrivate *priv = lupus_editable_label_get_instance_private(LUPUS_EDITABLE_LABEL(user_data));

    gchar const *label_string = gtk_label_get_text(GTK_LABEL(priv->label));
    gchar const *entry_string = gtk_entry_get_text(GTK_ENTRY(priv->entry));

    if (g_strcmp0(label_string, entry_string) != 0) {
        priv->text = (gchar *) entry_string;
        gtk_label_set_text(GTK_LABEL(priv->label), entry_string);
        g_signal_emit(user_data, signals[EDITED], 0, entry_string);
    }

    gtk_popover_popdown(GTK_POPOVER(priv->popover));
}

static gboolean label_clicked_callback(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    LupusEditableLabelPrivate *priv = lupus_editable_label_get_instance_private(LUPUS_EDITABLE_LABEL(user_data));

    gtk_entry_set_text(GTK_ENTRY(priv->entry), priv->text);
    gtk_popover_popup(GTK_POPOVER(priv->popover));

    return FALSE;
}

static void lupus_editable_label_constructed(GObject *object) {
    LupusEditableLabelPrivate *priv = lupus_editable_label_get_instance_private(LUPUS_EDITABLE_LABEL(object));

    priv->entry = gtk_entry_new();
    priv->button = gtk_button_new_from_icon_name("gtk-apply", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(priv->button, "clicked", G_CALLBACK(confirm_callback), object);

    priv->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(priv->box), priv->entry, 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(priv->box), priv->button, 0, 1, 0);

    priv->label = gtk_label_new(priv->text);
    priv->popover = gtk_popover_new(priv->label);
    gtk_popover_set_position(GTK_POPOVER(priv->popover), GTK_POS_BOTTOM);
    gtk_container_add(GTK_CONTAINER(priv->popover), priv->box);

    gtk_widget_show_all(priv->box);

    gtk_container_add(GTK_CONTAINER(object), priv->label);

    g_signal_connect(object, "button-release-event", G_CALLBACK(label_clicked_callback), object);
}

static void lupus_editable_label_init(LupusEditableLabel *instance) {}

static void lupus_editable_label_class_init(LupusEditableLabelClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->set_property = lupus_editable_label_set_property;
    object_class->get_property = lupus_editable_label_get_property;
    object_class->constructed = lupus_editable_label_constructed;

    signals[EDITED] = g_signal_new(
            "edited",
            LUPUS_TYPE_EDITABLE_LABEL,
            G_SIGNAL_RUN_LAST,
            0, NULL, NULL, NULL,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);

    obj_properties[PROP_TEXT] = g_param_spec_string(
            "text",
            "Text",
            "Text of the label",
            "UNKNOWN",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

LupusEditableLabel *lupus_editable_label_new(gchar *text) {
    return g_object_new(LUPUS_TYPE_EDITABLE_LABEL,
                        "text", text,
                        NULL);
}