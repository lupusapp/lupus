#include "../include/lupus_mainheaderbar.h"
#include "../include/lupus.h"
#include "../include/lupus_editablelabel.h"
#include "../include/lupus_main.h"
#include "../include/utils.h"

struct _LupusMainHeaderBar {
    GtkHeaderBar parent_instance;

    Tox const *tox;
    LupusMain *main;

    GtkButton *profile;
    GtkImage *profile_image;
    GtkPopover *popover;
    GtkGrid *grid;
    GtkFrame *frame;
    GtkImage *profile_image_big;
    LupusEditableLabel *name, *status_message;
};

G_DEFINE_TYPE(LupusMainHeaderBar, lupus_mainheaderbar, GTK_TYPE_HEADER_BAR)

enum { PROP_TOX = 1, PROP_MAIN, N_PROPERTIES };

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void profile_clicked_cb(LupusMainHeaderBar *instance) {
    gtk_popover_popup(instance->popover);
}

static void submit_name_cb(LupusEditableLabel *editablelabel,
                           gchar const *value, LupusMainHeaderBar *instance) {
    if (!tox_self_set_name((Tox *)instance->tox, (guint8 *)value, strlen(value),
                           NULL)) {
        lupus_error(instance, "Cannot set name.");
        return;
    }

    /* FIXME: return value from save */
    g_signal_emit_by_name(instance->main, "save", NULL);

    GValue val = G_VALUE_INIT;
    g_value_init(&val, G_TYPE_STRING); // NOLINT
    g_value_set_string(&val, value);

    g_object_set_property(G_OBJECT(editablelabel), "value", &val); // NOLINT
}

static void submit_status_message_cb(LupusEditableLabel *editablelabel,
                                     gchar const *value,
                                     LupusMainHeaderBar *instance) {
    if (!tox_self_set_status_message((Tox *)instance->tox, (guint8 *)value,
                                     strlen(value), NULL)) {
        lupus_error(instance, "Cannot set status message.");
        return;
    }

    /* FIXME: return value from save */
    g_signal_emit_by_name(instance->main, "save", NULL);

    GValue val = G_VALUE_INIT;
    g_value_init(&val, G_TYPE_STRING); // NOLINT
    g_value_set_string(&val, value);

    g_object_set_property(G_OBJECT(editablelabel), "value", &val); // NOLINT
}

static void lupus_mainheaderbar_set_property(LupusMainHeaderBar *instance,
                                             guint property_id,
                                             GValue const *value,
                                             GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_MAIN:
        instance->main = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_mainheaderbar_get_property(LupusMainHeaderBar *instance,
                                             guint property_id, GValue *value,
                                             GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, (gpointer)instance->tox);
        break;
    case PROP_MAIN:
        g_value_set_pointer(value, (gpointer)instance->main);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_mainheaderbar_constructed(LupusMainHeaderBar *instance) {
    gsize name_size = tox_self_get_name_size(instance->tox);
    gsize status_message_size = tox_self_get_status_message_size(instance->tox);

    guint8 name[name_size];
    guint8 status_message[status_message_size];

    tox_self_get_name(instance->tox, name);
    tox_self_get_status_message(instance->tox, status_message);

    name[name_size] = 0;
    status_message[status_message_size] = 0;

    /* FIXME: dispose */
    instance->name = lupus_editablelabel_new((gchar *)name);
    instance->status_message = lupus_editablelabel_new((gchar *)status_message);

    g_signal_connect(instance->name, "submit", G_CALLBACK(submit_name_cb),
                     instance);
    g_signal_connect(instance->status_message, "submit",
                     G_CALLBACK(submit_status_message_cb), instance);

    gtk_grid_attach(instance->grid, GTK_WIDGET(instance->name), 1, 0, 2, 1);
    gtk_grid_attach(instance->grid, GTK_WIDGET(instance->status_message), 1, 2,
                    2, 3);

    G_OBJECT_CLASS(lupus_mainheaderbar_parent_class) // NOLINT
        ->constructed(G_OBJECT(instance));           // NOLINT
}

static void lupus_mainheaderbar_class_init(LupusMainHeaderBarClass *class) {
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(class), LUPUS_RESOURCES "/mainheaderbar.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainHeaderBar, profile);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainHeaderBar, profile_image);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainHeaderBar, popover);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainHeaderBar, grid);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainHeaderBar, frame);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainHeaderBar, profile_image_big);

    G_OBJECT_CLASS(class)->set_property = // NOLINT
        lupus_mainheaderbar_set_property;
    G_OBJECT_CLASS(class)->get_property = // NOLINT
        lupus_mainheaderbar_get_property;
    G_OBJECT_CLASS(class)->constructed = // NOLINT
        lupus_mainheaderbar_constructed;

    obj_properties[PROP_TOX] = g_param_spec_pointer(
        "tox", "Tox", "Tox profile.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    obj_properties[PROP_MAIN] = g_param_spec_pointer(
        "main", "Main", "LupusMain parent.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    g_object_class_install_properties(G_OBJECT_CLASS(class), // NOLINT
                                      N_PROPERTIES, obj_properties);
}

static void lupus_mainheaderbar_init(LupusMainHeaderBar *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect_swapped(instance->profile, "clicked",
                             G_CALLBACK(profile_clicked_cb), instance);

    /* TODO: frame */
    gtk_frame_set_label(instance->frame, NULL);
}

LupusMainHeaderBar *lupus_mainheaderbar_new(Tox const *tox,
                                            LupusMain const *main) {
    return g_object_new(LUPUS_TYPE_MAINHEADERBAR, "tox", tox, "main", main,
                        NULL);
}