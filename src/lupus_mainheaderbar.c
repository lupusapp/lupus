#include "../include/lupus_mainheaderbar.h"
#include "../include/lupus.h"
#include "../include/lupus_editablelabel.h"
#include "../include/lupus_main.h"
#include "../include/utils.h"
#include <sodium/utils.h>

/*
 * TODO(ogromny): avatar, status
 * Maybe an indicator if offline
 */

struct _LupusMainHeaderBar {
    GtkHeaderBar parent_instance;

    Tox const *tox;
    LupusMain *main;
    gint status;
    gint connection;

    GtkButton *profile;
    GtkImage *profile_image;
    GtkMenu *profile_popover;
    GtkMenuItem *profile_popover_none, *profile_popover_away,
        *profile_popover_busy, *profile_popover_toxid;
    GtkPopover *popover;
    GtkButton *profile_bigger;
    GtkImage *profile_bigger_image;
    GtkBox *vbox;
    LupusEditableLabel *name, *status_message;
};

G_DEFINE_TYPE(LupusMainHeaderBar, lupus_mainheaderbar, GTK_TYPE_HEADER_BAR)

enum { PROP_TOX = 1, PROP_MAIN, PROP_STATUS, PROP_CONNECTION, N_PROPERTIES };

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

#define TOXID_DIALOG_MARGIN 5

static void profile_clicked_cb(LupusMainHeaderBar *instance) {
    gtk_popover_popup(instance->popover);
}

static gboolean profile_right_clicked_cb(GtkWidget *widget, // NOLINT
                                         GdkEvent *event,
                                         LupusMainHeaderBar *instance) {
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        gtk_menu_popup_at_pointer(instance->profile_popover, event);
    }
    return FALSE;
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

static void change_status_cb(GtkMenuItem *menuitem,
                             LupusMainHeaderBar *instance) {
    if (menuitem == instance->profile_popover_none) {
        g_object_set(instance, "status", TOX_USER_STATUS_NONE, NULL);
    } else if (menuitem == instance->profile_popover_away) {
        g_object_set(instance, "status", TOX_USER_STATUS_AWAY, NULL);
    } else if (menuitem == instance->profile_popover_busy) {
        g_object_set(instance, "status", TOX_USER_STATUS_BUSY, NULL);
    }
}

static void toxid_cb(GtkMenuItem *menuitem, // NOLINT
                     LupusMainHeaderBar *instance) {
    GtkDialog *dialog = GTK_DIALOG(g_object_new(
        GTK_TYPE_DIALOG, "use-header-bar", TRUE, "title", "ToxID", NULL));

    gtk_container_remove(
        GTK_CONTAINER(dialog),
        gtk_container_get_children(GTK_CONTAINER(dialog))->data);

    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, TOXID_DIALOG_MARGIN);
    gtk_container_add(GTK_CONTAINER(dialog), GTK_WIDGET(box));
    gtk_widget_set_margin_top(box, TOXID_DIALOG_MARGIN);
    gtk_widget_set_margin_end(box, TOXID_DIALOG_MARGIN);
    gtk_widget_set_margin_bottom(box, TOXID_DIALOG_MARGIN);
    gtk_widget_set_margin_start(box, TOXID_DIALOG_MARGIN);

    guint8 tox_address_bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(instance->tox, tox_address_bin);
    gchar tox_address_hex[TOX_ADDRESS_SIZE * 2 + 1];
    sodium_bin2hex(tox_address_hex, sizeof(tox_address_hex), tox_address_bin,
                   sizeof(tox_address_bin));

    GtkWidget *label = gtk_label_new(tox_address_hex);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);

    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(dialog));

    gtk_dialog_run(dialog);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void lupus_mainheaderbar_set_property(LupusMainHeaderBar *instance,
                                             guint property_id,
                                             GValue const *value,
                                             GParamSpec *pspec) {
    gchar static const *class_name[] = {
        "profile--none",
        "profile--away",
        "profile--busy",
        "profile--offline",
    };

    switch (property_id) {
    case PROP_TOX:
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_MAIN:
        instance->main = g_value_get_pointer(value);
        break;
    case PROP_STATUS: {
        instance->status = g_value_get_int(value);

        /*
         * Enable all status menuitem except the selected status
         */
        //        gtk_widget_set_sensitive(GTK_WIDGET(instance->profile_popover_none),
        //                                 TRUE);
        //        gtk_widget_set_sensitive(GTK_WIDGET(instance->profile_popover_away),
        //                                 TRUE);
        //        gtk_widget_set_sensitive(GTK_WIDGET(instance->profile_popover_busy),
        //                                 TRUE);
        int static const offset[] = {
            offsetof(LupusMainHeaderBar, profile_popover_none),
            offsetof(LupusMainHeaderBar, profile_popover_away),
            offsetof(LupusMainHeaderBar, profile_popover_busy),
        };
        gpointer actual =
            (instance->status == TOX_USER_STATUS_BUSY + 1)
                ? NULL
                : *(gpointer *)((gchar *)instance + offset[instance->status]);
        for (gsize i = 0, j = G_N_ELEMENTS(offset); i < j; ++i) {
            gpointer item = *(gpointer *)((gchar *)instance + offset[i]);
            gtk_widget_set_sensitive(GTK_WIDGET(item),
                                     (item == actual) ? FALSE : TRUE);
        }

        if (instance->status != TOX_USER_STATUS_BUSY + 1) {
            tox_self_set_status((Tox *)instance->tox, instance->status);
        }

        if (instance->connection == TOX_CONNECTION_NONE) {
            return;
        }

        remove_class_with_prefix(instance->profile, "profile--");
        remove_class_with_prefix(instance->profile_bigger, "profile--");

        gtk_style_context_add_class(
            gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
            class_name[instance->status]);
        gtk_style_context_add_class(
            gtk_widget_get_style_context(GTK_WIDGET(instance->profile_bigger)),
            class_name[instance->status]);
        break;
    }
    case PROP_CONNECTION: {
        /* if connection is already none, or if it will become none */
        if (instance->connection == TOX_CONNECTION_NONE ||
            g_value_get_int(value)) {
            remove_class_with_prefix(instance->profile, "profile--");
            remove_class_with_prefix(instance->profile_bigger, "profile--");

            gtk_style_context_add_class(
                gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
                class_name[instance->status]);
            gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(
                                            instance->profile_bigger)),
                                        class_name[instance->status]);
        }

        instance->connection = g_value_get_int(value);
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
    case PROP_CONNECTION:
        g_value_set_int(value, instance->connection);
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

    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->name), FALSE, FALSE,
                       0);
    gtk_box_pack_start(instance->vbox,
                       gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), TRUE,
                       TRUE, 0);
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->status_message),
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
                                         LupusMainHeaderBar, profile_popover);
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

    obj_properties[PROP_STATUS] = g_param_spec_int(
        "status", "Status", "Tox status.", TOX_USER_STATUS_NONE,
        TOX_USER_STATUS_BUSY + 1, TOX_USER_STATUS_BUSY + 1, G_PARAM_READWRITE);

    obj_properties[PROP_CONNECTION] =
        g_param_spec_int("connection", "Connection", "Tox connection status.",
                         TOX_CONNECTION_NONE, TOX_CONNECTION_UDP,
                         TOX_CONNECTION_NONE, G_PARAM_READWRITE);

    g_object_class_install_properties(G_OBJECT_CLASS(class), // NOLINT
                                      N_PROPERTIES, obj_properties);
}

static void lupus_mainheaderbar_init(LupusMainHeaderBar *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    GtkMenuItem **item[] = {
        &instance->profile_popover_none,
        &instance->profile_popover_away,
        &instance->profile_popover_busy,
        &instance->profile_popover_toxid,
    };
    gchar *svg[] = {
        LUPUS_RESOURCES "/status_none.svg",
        LUPUS_RESOURCES "/status_away.svg",
        LUPUS_RESOURCES "/status_busy.svg",
        LUPUS_RESOURCES "/biometric.svg",
    };
    gchar *label[] = {"Online", "Away", "Busy", "ToxID"};

    for (gsize i = 0, j = G_N_ELEMENTS(label); i < j; ++i) {
        GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
        gtk_box_pack_start(box, gtk_image_new_from_resource(svg[i]), FALSE,
                           TRUE, 0);
        gtk_box_pack_start(box, gtk_label_new(label[i]), TRUE, TRUE, 0);

        gtk_container_add(
            GTK_CONTAINER(*item[i] = GTK_MENU_ITEM(gtk_menu_item_new())),
            GTK_WIDGET(box));

        gtk_menu_shell_append(GTK_MENU_SHELL(instance->profile_popover),
                              GTK_WIDGET(*item[i]));

        g_signal_connect(*item[i], "activate",
                         (gpointer)(i == j - 1 ? toxid_cb : change_status_cb),
                         instance);
    }

    gtk_menu_shell_insert(GTK_MENU_SHELL(instance->profile_popover),
                          GTK_WIDGET(gtk_separator_menu_item_new()),
                          G_N_ELEMENTS(item) - 1);

    gtk_widget_show_all(GTK_WIDGET(instance->profile_popover));

    g_signal_connect(instance->profile, "button-press-event",
                     G_CALLBACK(profile_right_clicked_cb), instance);
    g_signal_connect_swapped(instance->profile, "clicked",
                             G_CALLBACK(profile_clicked_cb), instance);
}

LupusMainHeaderBar *lupus_mainheaderbar_new(Tox const *tox,
                                            LupusMain const *main, gint status,
                                            gint connection) {
    return g_object_new(LUPUS_TYPE_MAINHEADERBAR, "tox", tox, "main", main,
                        "status", status, "connection", connection, NULL);
}