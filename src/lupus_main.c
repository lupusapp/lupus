#include "../include/lupus_main.h"
#include "../include/lupus.h"
#include "../include/lupus_mainfriendlist.h"
#include "../include/lupus_mainheaderbar.h"
#include "../include/utils.h"
#include <sodium.h>

struct _LupusMain {
    GtkApplicationWindow parent_instance;

    guint tox_iteration;

    Tox const *tox;
    char const *profile_filename;
    char const *profile_password;

    LupusMainHeaderBar *main_header_bar;
    GtkBox *box;
    LupusMainFriendList *main_friend_list;
};

G_DEFINE_TYPE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

#define TOX_PORT 33445
#define SEPARATOR_MARGIN 5

typedef struct DHT_node {
    gchar const *ip;
    guint16 port;
    gchar const key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    guchar const key_bin[TOX_PUBLIC_KEY_SIZE];
} DHT_node;

static DHT_node nodes[] = {
    {"85.172.30.117", TOX_PORT,
     "8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832", 0},
    {"95.31.18.227", TOX_PORT,
     "257744DBF57BE3E117FE05D145B5F806089428D4DCE4E3D0D50616AA16D9417E", 0},
    {"94.45.70.19", TOX_PORT,
     "CE049A748EB31F0377F94427E8E3D219FC96509D4F9D16E181E956BC5B1C4564", 0},
    {"46.229.52.198", TOX_PORT,
     "813C8F4187833EF0655B10F7752141A352248462A567529A38B6BBF73E979307", 0},
};

enum {
    PROP_TOX = 1,
    PROP_PROFILE_FILENAME,
    PROP_PROFILE_PASSWORD,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

enum { SAVE, DISCONNECT, CONNECT, LAST_SIGNAL };

static guint signals[LAST_SIGNAL];

static gboolean iterate(LupusMain *instance) {
    tox_iterate((Tox *)instance->tox, instance);
    return TRUE;
}

static void save_cb(LupusMain *instance) {
    tox_save((Tox *)instance->tox, instance->profile_filename,
             instance->profile_password, GTK_WINDOW(instance), FALSE);
}

static void disconnect_cb(LupusMain *instance) {
    if (instance->tox_iteration) {
        g_source_remove(instance->tox_iteration);
        instance->tox_iteration = 0;
    }
}

static void connect_cb(LupusMain *instance) {
    if (!instance->tox_iteration) {
        instance->tox_iteration =
            g_timeout_add(tox_iteration_interval(instance->tox),
                          G_SOURCE_FUNC(iterate), instance);
    }
}

static void lupus_main_set_property(LupusMain *instance, guint property_id,
                                    GValue const *value, GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_PROFILE_FILENAME:
        instance->profile_filename = g_value_dup_string(value);
        break;
    case PROP_PROFILE_PASSWORD:
        instance->profile_password = g_value_dup_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_main_get_property(LupusMain *instance, guint property_id,
                                    GValue *value, GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, (gpointer)instance->tox);
        break;
    case PROP_PROFILE_FILENAME:
        g_value_set_string(value, instance->profile_filename);
        break;
    case PROP_PROFILE_PASSWORD:
        g_value_set_string(value, instance->profile_password);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void connection_status_cb(Tox *tox, // NOLINT
                                 TOX_CONNECTION connection_status,
                                 LupusMain *instance) {
    gint status;
    g_object_get(instance->main_header_bar, "status", &status, NULL);

    switch (connection_status) {
    case TOX_CONNECTION_UDP:
    case TOX_CONNECTION_TCP:
        /* Return if status is already online,busy or away */
        if (status != TOX_USER_STATUS_BUSY + 1) {
            return;
        }

        /* FIXME: last_status */
        g_object_set(instance->main_header_bar, "status", TOX_USER_STATUS_NONE,
                     NULL);
        break;
    default:
        /* Return if status is already offline */
        if (status == TOX_USER_STATUS_BUSY + 1) {
            return;
        }

        g_object_set(instance->main_header_bar, "status",
                     TOX_USER_STATUS_BUSY + 1, NULL);
    }
}

static void lupus_main_constructed(LupusMain *instance) {
    gtk_window_set_titlebar(
        GTK_WINDOW(instance),
        GTK_WIDGET(instance->main_header_bar = lupus_mainheaderbar_new(
                       instance->tox, instance, TOX_USER_STATUS_BUSY + 1)));

    gtk_box_pack_start(
        instance->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0)),
        GTK_WIDGET(instance->main_friend_list =
                       lupus_mainfriendlist_new(instance->tox, instance)),
        FALSE, TRUE, 0);

    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_top(separator, SEPARATOR_MARGIN);
    gtk_widget_set_margin_bottom(separator, SEPARATOR_MARGIN);
    gtk_box_pack_start(instance->box, separator, FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(instance), GTK_WIDGET(instance->box));
    gtk_widget_show_all(GTK_WIDGET(instance));

    /* Bootstrap */
    for (gsize i = 0, j = G_N_ELEMENTS(nodes); i < j; ++i) {
        sodium_hex2bin((guchar *)nodes[i].key_bin, sizeof(nodes[i].key_bin),
                       nodes[i].key_hex, sizeof(nodes[i].key_hex) - 1, NULL,
                       NULL, NULL);
        if (!tox_bootstrap((Tox *)instance->tox, nodes[i].ip, nodes[i].port,
                           nodes[i].key_bin, NULL)) {
            g_warning("Cannot bootstrap %s.", nodes[i].ip);
        }
    }
    /* Tox Callbacks */
    tox_callback_self_connection_status(
        (Tox *)instance->tox,
        (tox_self_connection_status_cb *)connection_status_cb);

    connect_cb(instance);

    G_OBJECT_CLASS(lupus_main_parent_class) // NOLINT
        ->constructed(G_OBJECT(instance));  // NOLINT
}

static void lupus_main_class_init(LupusMainClass *class) {
    G_OBJECT_CLASS(class)->set_property = lupus_main_set_property; // NOLINT
    G_OBJECT_CLASS(class)->get_property = lupus_main_get_property; // NOLINT
    G_OBJECT_CLASS(class)->constructed = lupus_main_constructed;   // NOLINT

    obj_properties[PROP_TOX] = g_param_spec_pointer(
        "tox", "Tox", "Tox profile.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT
    obj_properties[PROP_PROFILE_FILENAME] = g_param_spec_string(
        "profile-filename", "Profile filename", "Filename of the tox profile.",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT
    obj_properties[PROP_PROFILE_PASSWORD] = g_param_spec_string(
        "profile-password", "Profile password", "Password of the tox profile.",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    g_object_class_install_properties(G_OBJECT_CLASS(class), // NOLINT
                                      N_PROPERTIES, obj_properties);

    signals[SAVE] = g_signal_new("save", LUPUS_TYPE_MAIN, G_SIGNAL_RUN_LAST, 0,
                                 NULL, NULL, NULL, G_TYPE_NONE, 0); // NOLINT
    signals[DISCONNECT] =
        g_signal_new("disconnect", LUPUS_TYPE_MAIN, G_SIGNAL_RUN_LAST, 0, NULL,
                     NULL, NULL, G_TYPE_NONE, 0); // NOLINT
    signals[CONNECT] =
        g_signal_new("connect", LUPUS_TYPE_MAIN, G_SIGNAL_RUN_LAST, 0, NULL,
                     NULL, NULL, G_TYPE_NONE, 0); // NOLINT
}

static void lupus_main_init(LupusMain *instance) {
    g_signal_connect(instance, "save", G_CALLBACK(save_cb), NULL);
    g_signal_connect(instance, "disconnect", G_CALLBACK(disconnect_cb), NULL);
    g_signal_connect(instance, "connect", G_CALLBACK(connect_cb), NULL);
}

LupusMain *lupus_main_new(GtkApplication *application, Tox const *tox,
                          gchar const *profile_filename,
                          gchar const *profile_password) {
    return g_object_new(LUPUS_TYPE_MAIN, "application", application, "tox", tox,
                        "profile-filename", profile_filename,
                        "profile-password", profile_password, NULL);
}