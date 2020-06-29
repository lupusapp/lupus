#include "../include/lupus_objectfriend.h"
#include "../include/lupus_objectself.h"
#include "glibconfig.h"
#include "include/lupus.h"
#include <sodium/utils.h>
#include <stdint.h>
#include <tox/tox.h>

struct _LupusObjectFriend {
    GObject parent_instance;

    LupusObjectSelf *objectself;
    guint32 friend_number;
    gchar *avatar_hash;

    gchar *name;
    gchar *status_message;
    gchar *public_key;
};

G_DEFINE_TYPE(LupusObjectFriend, lupus_objectfriend, G_TYPE_OBJECT)

typedef enum {
    PROP_OBJECTSELF = 1,
    PROP_FRIEND_NUMBER,
    PROP_NAME,
    PROP_STATUS_MESSAGE,
    PROP_PUBLIC_KEY,
    PROP_AVATAR_HASH,
    N_PROPERTIES,
} LupusObjectFriendProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

typedef enum {
    REFRESH_AVATAR,
    LAST_SIGNAL,
} LupusObjectFriendSignal;
static guint signals[LAST_SIGNAL];

static LupusObjectFriend *get_instance_from_objectself(LupusObjectSelf *user_data, guint32 friend_number)
{
    LupusObjectSelf *objectself = LUPUS_OBJECTSELF(user_data);
    GHashTable *objectfriends;

    g_object_get(objectself, "objectfriends", &objectfriends, NULL);
    gpointer key = GUINT_TO_POINTER(friend_number);

    return LUPUS_OBJECTFRIEND(g_hash_table_lookup(objectfriends, key));
}

static void name_cb(Tox *tox, guint32 friend_number, guint8 const *name, gsize length, gpointer user_data)
{
    LupusObjectSelf *objectself = LUPUS_OBJECTSELF(user_data);
    LupusObjectFriend *instance = get_instance_from_objectself(objectself, friend_number);

    gchar *new_name = g_strndup((gchar *)name, length);
    if (!g_strcmp0(instance->name, new_name)) {
        free(new_name);
        return;
    }

    g_free(instance->name);
    instance->name = new_name;

    g_object_notify_by_pspec(G_OBJECT(instance), obj_properties[PROP_NAME]);
    g_signal_emit_by_name(objectself, "save");
}

static void status_message_cb(Tox *tox, guint32 friend_number, guint8 const *status_message, gsize length,
                              gpointer user_data)
{
    LupusObjectSelf *objectself = LUPUS_OBJECTSELF(user_data);
    LupusObjectFriend *instance = get_instance_from_objectself(objectself, friend_number);

    gchar *new_status_message = g_strndup((gchar *)status_message, length);
    if (!g_strcmp0(instance->status_message, new_status_message)) {
        free(new_status_message);
        return;
    }

    g_free(instance->status_message);
    instance->status_message = new_status_message;

    g_object_notify_by_pspec(G_OBJECT(instance), obj_properties[PROP_STATUS_MESSAGE]);
    g_signal_emit_by_name(objectself, "save");
}

static void load_avatar_hash(LupusObjectFriend *instance)
{
    instance->avatar_hash = NULL;

    gchar *avatar_directory = g_strconcat(LUPUS_TOX_DIR, "avatars/", NULL);
    if (!g_file_test(avatar_directory, G_FILE_TEST_IS_DIR)) {
        g_free(avatar_directory);
        return;
    }

    gsize profile_avatar_filename_size = strlen(avatar_directory) + tox_public_key_size() * 2 + 4 + 1;
    gchar profile_avatar_filename[profile_avatar_filename_size];
    memset(profile_avatar_filename, 0, profile_avatar_filename_size);
    g_strlcat(profile_avatar_filename, avatar_directory, profile_avatar_filename_size);
    g_strlcat(profile_avatar_filename, instance->public_key, profile_avatar_filename_size);
    g_strlcat(profile_avatar_filename, ".png", profile_avatar_filename_size);
    g_free(avatar_directory);

    if (!g_file_test(profile_avatar_filename, G_FILE_TEST_EXISTS)) {
        return;
    }

    gchar *contents;
    gsize length;
    g_file_get_contents(profile_avatar_filename, &contents, &length, NULL);

    guint8 avatar_hash_bin[tox_hash_length()];
    tox_hash(avatar_hash_bin, (guint8 *)contents, length);
    gchar avatar_hash_hex[tox_hash_length() * 2 + 1];
    sodium_bin2hex(avatar_hash_hex, sizeof(avatar_hash_hex), avatar_hash_bin, sizeof(avatar_hash_bin));
    instance->avatar_hash = g_ascii_strup(avatar_hash_hex, -1);

    g_free(contents);

    g_object_notify_by_pspec(instance, obj_properties[PROP_AVATAR_HASH]);
}

static void lupus_objectfriend_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusObjectFriend *instance = LUPUS_OBJECTFRIEND(object);

    switch ((LupusObjectFriendProperty)property_id) {
    case PROP_OBJECTSELF:
        instance->objectself = g_value_get_pointer(value);
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
    case PROP_OBJECTSELF:
        g_value_set_pointer(value, instance->objectself);
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
    case PROP_PUBLIC_KEY:
        g_value_set_string(value, instance->public_key);
        break;
    case PROP_AVATAR_HASH:
        g_value_set_string(value, instance->avatar_hash);
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
    g_free(instance->public_key);
    g_free(instance->avatar_hash);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_objectfriend_parent_class);
    object_class->finalize(object);
}

static void lupus_objectfriend_constructed(GObject *object)
{
    LupusObjectFriend *instance = LUPUS_OBJECTFRIEND(object);
    Tox *tox = NULL;
    g_object_get(instance->objectself, "tox", &tox, NULL);

    guint8 public_key_bin[tox_public_key_size()];
    tox_friend_get_public_key(tox, instance->friend_number, public_key_bin, NULL);
    gchar public_key_hex[tox_public_key_size() * 2 + 1];
    sodium_bin2hex(public_key_hex, sizeof(public_key_hex), public_key_bin, sizeof(public_key_bin));
    instance->public_key = g_ascii_strup(public_key_hex, -1);

    gsize name_size = tox_friend_get_name_size(tox, instance->friend_number, NULL);
    if (name_size) {
        instance->name = g_malloc0(name_size + 1);
        tox_friend_get_name(tox, instance->friend_number, (guint8 *)instance->name, NULL);
    } else {
        instance->name = g_ascii_strup(instance->public_key, -1);
    }

    gsize status_message_size = tox_friend_get_status_message_size(tox, instance->friend_number, NULL);
    if (status_message_size) {
        instance->status_message = g_malloc0(status_message_size + 1);
        tox_friend_get_status_message(tox, instance->friend_number, (guint8 *)instance->status_message, NULL);
    } else {
        instance->status_message = NULL;
    }

    load_avatar_hash(instance);

    g_signal_connect(instance, "refresh-avatar", G_CALLBACK(load_avatar_hash), NULL);

    tox_callback_friend_name(tox, name_cb);
    tox_callback_friend_status_message(tox, status_message_cb);

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

    obj_properties[PROP_OBJECTSELF] =
        g_param_spec_pointer("objectself", NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_FRIEND_NUMBER] =
        g_param_spec_uint("friend-number", NULL, NULL, 0, INT32_MAX, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_NAME] = g_param_spec_string("name", NULL, NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_STATUS_MESSAGE] = g_param_spec_string("status-message", NULL, NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_PUBLIC_KEY] = g_param_spec_string("public-key", NULL, NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_AVATAR_HASH] = g_param_spec_string("avatar-hash", NULL, NULL, NULL, G_PARAM_READABLE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    signals[REFRESH_AVATAR] =
        g_signal_new("refresh-avatar", LUPUS_TYPE_OBJECTFRIEND, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void lupus_objectfriend_init(LupusObjectFriend *instance) {}

LupusObjectFriend *lupus_objectfriend_new(LupusObjectSelf *objectself, guint32 friend_number)
{
    return g_object_new(LUPUS_TYPE_OBJECTFRIEND, "objectself", objectself, "friend-number", friend_number, NULL);
}
