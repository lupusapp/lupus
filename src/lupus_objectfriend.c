#include "../include/lupus_objectfriend.h"
#include "glibconfig.h"
#include <stdint.h>
#include <tox/tox.h>

struct _LupusObjectFriend {
    GObject parent_instance;

    Tox *tox;
    guint32 friend_number;

    gchar *name;
    gchar *status_message;
};

G_DEFINE_TYPE(LupusObjectFriend, lupus_objectfriend, G_TYPE_OBJECT)

typedef enum {
    PROP_TOX = 1,
    PROP_FRIEND_NUMBER,
    PROP_NAME,
    PROP_STATUS_MESSAGE,
    N_PROPERTIES,
} LupusObjectFriendProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void lupus_objectfriend_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusObjectFriend *instance = LUPUS_OBJECTFRIEND(object);

    switch ((LupusObjectFriendProperty)property_id) {
    case PROP_TOX:
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_FRIEND_NUMBER:
        instance->friend_number = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_objectfriend_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    LupusObjectFriend *instance = LUPUS_OBJECTFRIEND(object);

    switch ((LupusObjectFriendProperty)property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, instance->tox);
        break;
    case PROP_FRIEND_NUMBER:
        g_value_set_uint(value, instance->friend_number);
        break;
    case PROP_NAME:
        g_value_set_string(value, instance->name);
        break;
    case PROP_STATUS_MESSAGE:
        g_value_set_string(value, instance->status_message);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_objectfriend_finalize(GObject *object)
{
    LupusObjectFriend *instance = LUPUS_OBJECTFRIEND(object);

    g_free(instance->name);
    g_free(instance->status_message);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_objectfriend_parent_class);
    object_class->finalize(object);
}
static void lupus_objectfriend_constructed(GObject *object)
{
    LupusObjectFriend *instance = LUPUS_OBJECTFRIEND(object);

    gsize name_size = tox_friend_get_name_size(instance->tox, instance->friend_number, NULL);
    if (name_size) {
        instance->name = g_malloc0(name_size + 1);
        tox_friend_get_name(instance->tox, instance->friend_number, (guint8 *)instance->name, NULL);
    } else {
        instance->name = NULL;
    }

    gsize status_message_size = tox_friend_get_status_message_size(instance->tox, instance->friend_number, NULL);
    if (status_message_size) {
        instance->status_message = g_malloc0(status_message_size + 1);
        tox_friend_get_status_message(instance->tox, instance->friend_number, (guint8 *)instance->status_message, NULL);
    } else {
        instance->status_message = NULL;
    }

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_objectfriend_parent_class);
    object_class->constructed(object);
}

static void lupus_objectfriend_class_init(LupusObjectFriendClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_objectfriend_constructed;
    object_class->finalize = lupus_objectfriend_finalize;
    object_class->set_property = lupus_objectfriend_set_property;
    object_class->get_property = lupus_objectfriend_get_property;

    obj_properties[PROP_TOX] = g_param_spec_pointer("tox", NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_FRIEND_NUMBER] =
        g_param_spec_uint("friend-number", NULL, NULL, 0, INT32_MAX, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_NAME] = g_param_spec_string("name", NULL, NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_STATUS_MESSAGE] = g_param_spec_string("status-message", NULL, NULL, NULL, G_PARAM_READABLE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_objectfriend_init(LupusObjectFriend *instance) {}

LupusObjectFriend *lupus_objectfriend_new(Tox *tox, guint32 friend_number)
{
    return g_object_new(LUPUS_TYPE_OBJECTFRIEND, "tox", tox, "friend-number", friend_number, NULL);
}
