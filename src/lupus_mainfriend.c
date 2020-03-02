#include "../include/lupus_mainfriend.h"
#include "../include/lupus.h"
#include "../include/utils.h"
#include <sodium.h>

struct _LupusMainFriend {
    GtkEventBox parent_instance;

    Tox const *tox;
    LupusMain const *main;
    guint32 friend;

    GtkButton *profile;
    GtkImage *profile_image;
    GtkLabel *name, *status_message;
};

G_DEFINE_TYPE(LupusMainFriend, lupus_mainfriend, GTK_TYPE_EVENT_BOX)

enum { PROP_TOX = 1, PROP_MAIN, PROP_FRIEND, N_PROPERTIES };
enum { UPDATE, LAST_SIGNAL };

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};
static guint signals[LAST_SIGNAL];

static void update_cb(LupusMainFriend *instance, guint flags, GPtrArray *data) {
    if (flags & UPDATE_STATUS) { // NOLINT
        remove_class_with_prefix(instance->profile, "profile--");

        gchar static const *classes[] = {"profile--none", "profile--away",
                                         "profile--busy"};
        gtk_style_context_add_class(
            gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
            classes[GPOINTER_TO_INT(g_ptr_array_index(data, 0))]);
    }

    if (flags & UPDATE_NAME) { // NOLINT
        gchar const *const name = g_ptr_array_index(data, 0);

        if (g_strcmp0(gtk_label_get_text(instance->name), name)) {
            gtk_label_set_text(instance->name, name);
            gtk_widget_set_tooltip_text(GTK_WIDGET(instance->name), name);
        }
    }

    if (flags & UPDATE_STATUS_MESSAGE) { // NOLINT
        gchar const *const status_message = g_ptr_array_index(data, 0);

        if (g_strcmp0(gtk_label_get_text(instance->status_message),
                      status_message)) {
            gtk_label_set_text(instance->status_message, status_message);
            gtk_widget_set_tooltip_text(GTK_WIDGET(instance->status_message),
                                        status_message);
        }
    }

    if (flags & UPDATE_CONNECTION) { // NOLINT
        if (GPOINTER_TO_INT(g_ptr_array_index(data, 0)) ==
            TOX_CONNECTION_NONE) {
            remove_class_with_prefix(instance->profile, "profile--");
            gtk_style_context_add_class(
                gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
                "profile--offline");
        }
    }

    g_ptr_array_free(data, TRUE);
}

static void lupus_mainfriend_set_property(LupusMainFriend *instance,
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
    case PROP_FRIEND:
        instance->friend = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_mainfriend_get_property(LupusMainFriend *instance,
                                          guint property_id, GValue *value,
                                          GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, (gpointer)instance->tox);
        break;
    case PROP_MAIN:
        g_value_set_pointer(value, (gpointer)instance->main);
        break;
    case PROP_FRIEND:
        g_value_set_uint(value, instance->friend);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_mainfriend_constructed(LupusMainFriend *instance) {
    gsize name_size =
        tox_friend_get_name_size(instance->tox, instance->friend, NULL);
    gsize status_message_size = tox_friend_get_status_message_size(
        instance->tox, instance->friend, NULL);

    guint8 name[name_size];
    guint8 status_message[status_message_size];

    tox_friend_get_name(instance->tox, instance->friend, name, NULL);
    tox_friend_get_status_message(instance->tox, instance->friend,
                                  status_message, NULL);

    name[name_size] = 0;
    status_message[status_message_size] = 0;

    if (*name) {
        gtk_label_set_text(instance->name, (gchar *)name);
    } else {
        // FIXME: get address (with nospam and checksum)
        guint8 friend_address_bin[TOX_PUBLIC_KEY_SIZE];
        tox_friend_get_public_key(instance->tox, instance->friend,
                                  friend_address_bin, NULL);

        gchar friend_address_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        sodium_bin2hex(friend_address_hex, sizeof(friend_address_hex),
                       friend_address_bin, sizeof(friend_address_bin));

        gtk_label_set_text(instance->name, friend_address_hex);
    }

    gtk_label_set_text(instance->status_message, (*status_message)
                                                     ? (gchar *)status_message
                                                     : "Has not accepted yet.");

    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->name),
                                gtk_label_get_text(instance->name));
    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->status_message),
                                gtk_label_get_text(instance->status_message));

    gtk_style_context_add_class(
        gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
        "profile--offline");
    gtk_widget_set_can_focus(GTK_WIDGET(instance->profile), FALSE);

    /* FIXME: avatar */

    G_OBJECT_CLASS(lupus_mainfriend_parent_class) // NOLINT
        ->constructed(G_OBJECT(instance));        // NOLINT
}

static void lupus_mainfriend_class_init(LupusMainFriendClass *class) {
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(class), LUPUS_RESOURCES "/mainfriend.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainFriend, profile);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainFriend, profile_image);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainFriend, name);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainFriend, status_message);

    G_OBJECT_CLASS(class)->set_property = // NOLINT
        lupus_mainfriend_set_property;
    G_OBJECT_CLASS(class)->get_property = // NOLINT
        lupus_mainfriend_get_property;
    G_OBJECT_CLASS(class)->constructed = // NOLINT
        lupus_mainfriend_constructed;

    obj_properties[PROP_TOX] = g_param_spec_pointer(
        "tox", "Tox", "Tox profile.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    obj_properties[PROP_MAIN] = g_param_spec_pointer(
        "main", "Main", "LupusMain parent.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    obj_properties[PROP_FRIEND] =
        g_param_spec_uint("friend", "Friend", "Tox friend.", 0, UINT32_MAX, 0,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    g_object_class_install_properties(G_OBJECT_CLASS(class), // NOLINT
                                      N_PROPERTIES, obj_properties);

    signals[UPDATE] = g_signal_new(
        "update", LUPUS_TYPE_MAINFRIEND, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
        G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER); // NOLINT
}

static void lupus_mainfriend_init(LupusMainFriend *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect(instance, "update", G_CALLBACK(update_cb), NULL);
}

LupusMainFriend *lupus_mainfriend_new(Tox const *tox, LupusMain const *main,
                                      guint32 friend) {
    return g_object_new(LUPUS_TYPE_MAINFRIEND, "tox", tox, "main", main,
                        "friend", friend, NULL);
}