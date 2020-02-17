#include "../include/lupus_mainheaderbar.h"
#include "../include/lupus.h"
#include "../include/lupus_editablelabel.h"
#include "../include/lupus_main.h"
#include "../include/utils.h"

/*
 * TODO(ogromny): avatar, status
 */

struct _LupusMainHeaderBar {
    GtkHeaderBar parent_instance;

    Tox const *tox;
    LupusMain *main;
    gint status;

    GtkButton *profile;
    GtkImage *profile_image;
    GtkPopover *popover;
    GtkButton *profile_bigger;
    GtkImage *profile_bigger_image;
    GtkBox *vbox;
    LupusEditableLabel *name, *status_message;
};

G_DEFINE_TYPE(LupusMainHeaderBar, lupus_mainheaderbar, GTK_TYPE_HEADER_BAR)

enum { PROP_TOX = 1, PROP_MAIN, PROP_STATUS, N_PROPERTIES };

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
    case PROP_STATUS: {
        GtkStyleContext *profile_context =
            gtk_widget_get_style_context(GTK_WIDGET(instance->profile));
        GtkStyleContext *profile_bigger_context =
            gtk_widget_get_style_context(GTK_WIDGET(instance->profile_bigger));

        GList *profile_class = gtk_style_context_list_classes(profile_context);
        GList *profile_bigger_class =
            gtk_style_context_list_classes(profile_context);

        for (GList *a = profile_class; a != NULL; a = a->next) {
            if (g_str_has_prefix(a->data, "profile--")) {
                gtk_style_context_remove_class(profile_context, a->data);
            }
        }
        for (GList *a = profile_bigger_class; a != NULL; a = a->next) {
            if (g_str_has_prefix(a->data, "profile--")) {
                gtk_style_context_remove_class(profile_bigger_context, a->data);
            }
        }

        g_list_free(profile_class);
        g_list_free(profile_bigger_class);

        instance->status = g_value_get_int(value);

        switch (instance->status) {
        case TOX_USER_STATUS_NONE:
            gtk_style_context_add_class(profile_context, "profile--none");
            gtk_style_context_add_class(profile_bigger_context,
                                        "profile--none");
            break;
        case TOX_USER_STATUS_AWAY:
            gtk_style_context_add_class(profile_context, "profile--away");
            gtk_style_context_add_class(profile_bigger_context,
                                        "profile--away");
            break;
        case TOX_USER_STATUS_BUSY:
            gtk_style_context_add_class(profile_context, "profile--busy");
            gtk_style_context_add_class(profile_bigger_context,
                                        "profile--busy");
            break;
        default:
            gtk_style_context_add_class(profile_context, "profile--offline");
            gtk_style_context_add_class(profile_bigger_context,
                                        "profile--offline");
        }
        break;
    }
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
    case PROP_STATUS:
        g_value_set_int(value, instance->status);
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

    instance->name =
        lupus_editablelabel_new((gchar *)name, TOX_MAX_NAME_LENGTH);
    instance->status_message = lupus_editablelabel_new(
        (gchar *)status_message, TOX_MAX_STATUS_MESSAGE_LENGTH);

    g_signal_connect(instance->name, "submit", G_CALLBACK(submit_name_cb),
                     instance);
    g_signal_connect(instance->status_message, "submit",
                     G_CALLBACK(submit_status_message_cb), instance);

    gtk_box_pack_end(instance->vbox, GTK_WIDGET(instance->name), FALSE, FALSE,
                     0);
    gtk_box_pack_end(instance->vbox,
                     gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), TRUE, TRUE,
                     0);
    gtk_box_pack_end(instance->vbox, GTK_WIDGET(instance->status_message),
                     FALSE, FALSE, 0);

    gtk_widget_show_all(GTK_WIDGET(instance->vbox));

    gtk_widget_set_can_focus(GTK_WIDGET(instance->profile), FALSE);
    gtk_widget_set_can_focus(GTK_WIDGET(instance->profile_bigger), FALSE);

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
                                         LupusMainHeaderBar, profile_bigger);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(class), LupusMainHeaderBar, profile_bigger_image);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainHeaderBar, vbox);

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

    obj_properties[PROP_STATUS] =
        g_param_spec_int("status", "Status", "Tox status.", -1,
                         TOX_USER_STATUS_BUSY, -1, G_PARAM_READWRITE);

    g_object_class_install_properties(G_OBJECT_CLASS(class), // NOLINT
                                      N_PROPERTIES, obj_properties);
}

static void lupus_mainheaderbar_init(LupusMainHeaderBar *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect_swapped(instance->profile, "clicked",
                             G_CALLBACK(profile_clicked_cb), instance);
}

LupusMainHeaderBar *
lupus_mainheaderbar_new(Tox const *tox, LupusMain const *main, gint status) {
    return g_object_new(LUPUS_TYPE_MAINHEADERBAR, "tox", tox, "main", main,
                        "status", status, NULL);
}