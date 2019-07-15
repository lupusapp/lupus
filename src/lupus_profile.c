#include "../include/lupus_profile.h"
#include "../include/lupus.h"
#include "../include/lupus_editable_label.h"
#include "../include/utils.h"

struct _LupusProfile {
    GtkBox parent_instance;
};

typedef struct _LupusProfilePrivate LupusProfilePrivate;
struct _LupusProfilePrivate {
    Tox *tox;
    GtkBox *box;
};

G_DEFINE_TYPE_WITH_PRIVATE(LupusProfile, lupus_profile, GTK_TYPE_BOX)

enum {
    SAVE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

enum {
    PROP_TOX = 1,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void lupus_profile_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec) {
    LupusProfilePrivate *priv = lupus_profile_get_instance_private(LUPUS_PROFILE(object));

    switch (property_id) {
        case PROP_TOX:
            priv->tox = g_value_peek_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_profile_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
    LupusProfilePrivate *priv = lupus_profile_get_instance_private(LUPUS_PROFILE(object));

    switch (property_id) {
        case PROP_TOX:
            g_value_set_pointer(value, priv->tox);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void change_name_callback(LupusEditableLabel *editable, gchar *data, LupusProfile *profile) {
    LupusProfilePrivate *priv = lupus_profile_get_instance_private(profile);

    TOX_ERR_SET_INFO tox_err_set_info;
    tox_self_set_name(priv->tox, (uint8_t *) data, strlen(data), &tox_err_set_info);

    if (tox_err_set_info != TOX_ERR_SET_INFO_OK) {
        error_message(NULL, "<b>Error</b>: tox_self_set_name returned %d.", tox_err_set_info);
        return;
    }

    g_signal_emit_by_name(profile, "save");
}

static void change_status_message_callback(LupusEditableLabel *editable, gchar *data, LupusProfile *profile) {
    LupusProfilePrivate *priv = lupus_profile_get_instance_private(profile);

    TOX_ERR_SET_INFO tox_err_set_info;
    tox_self_set_status_message(priv->tox, (uint8_t *) data, strlen(data), &tox_err_set_info);

    if (tox_err_set_info != TOX_ERR_SET_INFO_OK) {
        error_message(NULL, "<b>Error</b>: tox_self_set_status_message returned %d.", tox_err_set_info);
        return;
    }

    g_signal_emit_by_name(profile, "save");
}

static void lupus_profile_constructed(GObject *object) {
    G_OBJECT_CLASS(lupus_profile_parent_class)->constructed(object);

    LupusProfilePrivate *priv = lupus_profile_get_instance_private(LUPUS_PROFILE(object));

    size_t name_size = tox_self_get_name_size(priv->tox);
    uint8_t name[name_size];
    tox_self_get_name(priv->tox, name);
    name[name_size] = '\0';

    LupusEditableLabel *name_label = lupus_editable_label_new((gchar *) name);

    size_t status_message_size = tox_self_get_status_message_size(priv->tox);
    uint8_t status_message[status_message_size];
    tox_self_get_status_message(priv->tox, status_message);
    status_message[status_message_size] = '\0';

    LupusEditableLabel *status_message_label = lupus_editable_label_new((gchar *) status_message);

    gtk_box_pack_start(GTK_BOX(priv->box), GTK_WIDGET(name_label), 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(priv->box), GTK_WIDGET(status_message_label), 1, 1, 0);

    g_signal_connect(name_label, "edited", G_CALLBACK(change_name_callback), object);
    g_signal_connect(status_message_label, "edited", G_CALLBACK(change_status_message_callback), object);
}

static void lupus_profile_init(LupusProfile *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));
}

static void lupus_profile_class_init(LupusProfileClass *class) {
    gchar *resource = g_strconcat(LUPUS_RESOURCES, "/profile.ui", NULL);
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), resource);
    g_free(resource);

    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), LupusProfile, box);

    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->set_property = lupus_profile_set_property;
    object_class->get_property = lupus_profile_get_property;
    object_class->constructed = lupus_profile_constructed;

    signals[SAVE] = g_signal_new(
            "save",
            LUPUS_TYPE_PROFILE,
            G_SIGNAL_RUN_LAST,
            0, NULL, NULL, NULL,
            G_TYPE_NONE, 0);

    obj_properties[PROP_TOX] = g_param_spec_pointer(
            "tox",
            "Tox",
            "Tox instance to use.",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

LupusProfile *lupus_profile_new(Tox *tox) {
    return g_object_new(LUPUS_TYPE_PROFILE,
                        "tox", tox,
                        NULL);
}