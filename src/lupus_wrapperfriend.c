#include "../include/lupus_wrapperfriend.h"
#include "../include/lupus_wrapper.h"
#include <sodium.h>

struct _LupusWrapperFriend {
    GObject parent_instance;

    gchar *last_avatar_hash_transmitted;

    LupusWrapper *wrapper;
    guint id;
    gchar *name;
    gchar *status_message;
    Tox_User_Status status;
    Tox_Connection connection;
    /* Last online */
    gchar *public_key;
    /* Typing */

    gchar *avatar_hash;
};

G_DEFINE_TYPE(LupusWrapperFriend, lupus_wrapperfriend, G_TYPE_OBJECT) // NOLINT

typedef enum {
    /*
     * I need to use a wrapper prop, because when I initiate the wrapper in
     * `login_cb` (`lupus_profilechooser.c`), the `constructed` function of
     * LupusWrapper will be called, and in this function, LupusWrapperFriend
     * `class_init` function will be called, and in this function, lupus_wrapper
     * is used, but it's undefined because LupusWrapper `constructed` has not
     * returned...
     */
    PROP_WRAPPER = 1,
    PROP_ID,
    PROP_NAME,
    PROP_STATUS_MESSAGE,
    PROP_STATUS,
    PROP_CONNECTION,
    PROP_LAST_AVATAR_HASH_TRANSMITTED,
    PROP_PUBLIC_KEY,
    PROP_AVATAR_HASH,
    N_PROPERTIES,
} LupusWrapperFriendProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

getter(wrapperfriend, WrapperFriend, id, guint);
getter(wrapperfriend, WrapperFriend, name, gchar *);
getter(wrapperfriend, WrapperFriend, status_message, gchar *);
getter(wrapperfriend, WrapperFriend, status, Tox_User_Status);
getter(wrapperfriend, WrapperFriend, connection, Tox_Connection);
getter(wrapperfriend, WrapperFriend, public_key, gchar *);
getter(wrapperfriend, WrapperFriend, avatar_hash, gchar *);
getter_setter(wrapperfriend, WrapperFriend, last_avatar_hash_transmitted, gchar *);

void lupus_wrapperfriend_set_avatar_hash(LupusWrapperFriend *instance)
{
    gchar *filename = g_strconcat(LUPUS_TOX_DIR, "avatars/", instance->public_key, ".png", NULL);

    gsize contents_length = 0;
    gchar *contents = NULL;
    g_file_get_contents(filename, &contents, &contents_length, NULL);

    g_free(filename);

    guint8 hash[TOX_HASH_LENGTH];
    tox_hash(hash, (guint8 *)contents, contents_length);

    gchar hash_hex[TOX_HASH_LENGTH * 2 + 1];
    sodium_bin2hex(hash_hex, sizeof(hash_hex), hash, sizeof(hash));
    instance->avatar_hash = g_strdup(hash_hex);

    GObject *gobject = G_OBJECT(instance); // NOLINT
    g_object_notify_by_pspec(gobject, obj_properties[PROP_AVATAR_HASH]);

    lupus_wrapper_save(instance->wrapper);
}

static void name_cb(Tox *tox, guint32 friend_number, guint8 const *name, gsize length, gpointer user_data) // NOLINT
{
    LupusWrapperFriend *instance = LUPUS_WRAPPERFRIEND(lupus_wrapper_get_friend(lupus_wrapper, friend_number));

    g_free(instance->name);
    instance->name = g_strdup((gchar *)name);

    GObject *object = G_OBJECT(instance); // NOLINT
    g_object_notify_by_pspec(object, obj_properties[PROP_NAME]);

    lupus_wrapper_save(instance->wrapper);
}

// NOLINTNEXTLINE
static void status_message_cb(Tox *tox, guint32 friend_number, const guint8 *message, gsize length, gpointer user_data)
{
    LupusWrapperFriend *instance = LUPUS_WRAPPERFRIEND(lupus_wrapper_get_friend(lupus_wrapper, friend_number));

    g_free(instance->status_message);
    instance->status_message = g_strdup((gchar *)message);

    GObject *object = G_OBJECT(instance); // NOLINT
    g_object_notify_by_pspec(object, obj_properties[PROP_STATUS_MESSAGE]);

    lupus_wrapper_save(instance->wrapper);
}

static void status_cb(Tox *tox, guint32 friend_number, TOX_USER_STATUS status, gpointer user_data) // NOLINT
{
    LupusWrapperFriend *instance = lupus_wrapper_get_friend(lupus_wrapper, friend_number);

    instance->status = status;

    GObject *object = G_OBJECT(instance); // NOLINT
    g_object_notify_by_pspec(object, obj_properties[PROP_STATUS]);
}

// NOLINTNEXTLINE
static void connection_cb(Tox *tox, guint32 friend_number, TOX_CONNECTION connection_status, gpointer user_data)
{
    LupusWrapperFriend *instance = lupus_wrapper_get_friend(lupus_wrapper, friend_number);

    instance->connection = connection_status;

    /*
     * Check if last avatar has been sent
     */
    if (connection_status != TOX_CONNECTION_NONE) {
        gchar *actual_hash = lupus_wrapper_get_avatar_hash(lupus_wrapper);

        if (g_strcmp0(actual_hash, instance->last_avatar_hash_transmitted)) {
            lupus_wrapper_send_avatar(lupus_wrapper, friend_number);
        }
    }

    g_object_notify_by_pspec(G_OBJECT(instance), // NOLINT
                             obj_properties[PROP_CONNECTION]);
}

static void lupus_wrapperfriend_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    LupusWrapperFriend *instance = LUPUS_WRAPPERFRIEND(object);

    switch ((LupusWrapperFriendProperty)property_id) {
    case PROP_WRAPPER:
        g_value_set_pointer(value, instance->wrapper);
        break;
    case PROP_ID:
        g_value_set_uint(value, instance->id);
        break;
    case PROP_NAME:
        g_value_set_string(value, instance->name);
        break;
    case PROP_STATUS_MESSAGE:
        g_value_set_string(value, instance->status_message);
        break;
    case PROP_STATUS:
        g_value_set_int(value, instance->status);
        break;
    case PROP_CONNECTION:
        g_value_set_int(value, instance->connection);
        break;
    case PROP_PUBLIC_KEY:
        g_value_set_string(value, instance->public_key);
        break;
    case PROP_AVATAR_HASH:
        g_value_set_string(value, instance->avatar_hash);
        break;
    case PROP_LAST_AVATAR_HASH_TRANSMITTED:
        g_value_set_string(value, instance->last_avatar_hash_transmitted);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_wrapperfriend_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    LupusWrapperFriend *instance = LUPUS_WRAPPERFRIEND(object);

    switch ((LupusWrapperFriendProperty)property_id) {
    case PROP_WRAPPER:
        instance->wrapper = LUPUS_WRAPPER(g_value_get_pointer(value));
        break;
    case PROP_ID:
        instance->id = g_value_get_uint(value);
        break;
    case PROP_LAST_AVATAR_HASH_TRANSMITTED:
        instance->last_avatar_hash_transmitted = (gchar *)g_value_get_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_wrapperfriend_finalize(GObject *object)
{
    LupusWrapperFriend *instance = LUPUS_WRAPPERFRIEND(object);

    g_free(instance->name);
    g_free(instance->status_message);
    g_free(instance->public_key);
    g_free(instance->avatar_hash);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_wrapperfriend_parent_class); // NOLINT
    parent_class->finalize(object);
}

static void lupus_wrapperfriend_constructed(GObject *object)
{
    LupusWrapperFriend *instance = LUPUS_WRAPPERFRIEND(object);

    Tox *tox = lupus_wrapper_get_tox(instance->wrapper);

    gsize name_size = tox_friend_get_name_size(tox, instance->id, NULL);
    if (name_size) {
        instance->name = g_malloc(name_size + 1);
        tox_friend_get_name(tox, instance->id, (guint8 *)instance->name, NULL);
        instance->name[name_size] = 0;
    } else {
        guint8 friend_address_bin[TOX_PUBLIC_KEY_SIZE];
        tox_friend_get_public_key(tox, instance->id, friend_address_bin, NULL);

        gchar friend_address_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        sodium_bin2hex(friend_address_hex, sizeof(friend_address_hex), friend_address_bin, sizeof(friend_address_bin));

        instance->name = g_ascii_strup(friend_address_hex, -1);
    }

    gsize status_message_size = tox_friend_get_status_message_size(tox, instance->id, NULL);
    if (status_message_size) {
        instance->status_message = g_malloc(status_message_size + 1);
        tox_friend_get_status_message(tox, instance->id, (guint8 *)instance->status_message, NULL);
        instance->status_message[status_message_size] = 0;
    }

    instance->status = tox_friend_get_status(tox, instance->id, NULL);

    instance->connection = tox_friend_get_connection_status(tox, instance->id, NULL);

    guint8 public_key[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, instance->id, public_key, NULL);
    gchar public_key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    sodium_bin2hex(public_key_hex, sizeof(public_key_hex), public_key, sizeof(public_key));
    instance->public_key = g_ascii_strup(public_key_hex, -1);

    instance->avatar_hash = NULL;

    instance->last_avatar_hash_transmitted = NULL;

    tox_callback_friend_name(tox, name_cb);
    tox_callback_friend_status_message(tox, status_message_cb);
    tox_callback_friend_status(tox, status_cb);
    tox_callback_friend_connection_status(tox, connection_cb);

    lupus_wrapperfriend_set_avatar_hash(instance);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_wrapperfriend_parent_class); // NOLINT
    parent_class->constructed(object);
}

static void lupus_wrapperfriend_class_init(LupusWrapperFriendClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class); // NOLINT
    object_class->constructed = lupus_wrapperfriend_constructed;
    object_class->finalize = lupus_wrapperfriend_finalize;
    object_class->set_property = lupus_wrapperfriend_set_property;
    object_class->get_property = lupus_wrapperfriend_get_property;

    gint param = G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY; // NOLINT
    obj_properties[PROP_WRAPPER] = g_param_spec_pointer("wrapper", "Wrapper", "Lupus Wrapper", param);
    obj_properties[PROP_ID] = g_param_spec_uint("id", "ID", "Friend ID", 0, UINT32_MAX, 0, param);

    gint param2 = G_PARAM_READABLE;
    obj_properties[PROP_NAME] = g_param_spec_string("name", "Name", "Friend name", NULL, param2);
    obj_properties[PROP_STATUS_MESSAGE] =
        g_param_spec_string("status-message", "Status message", "Friend status message", NULL, param2);
    obj_properties[PROP_STATUS] = g_param_spec_int("status", "Status", "Friend status", TOX_USER_STATUS_NONE,
                                                   TOX_USER_STATUS_BUSY, TOX_USER_STATUS_NONE, param2);
    obj_properties[PROP_CONNECTION] =
        g_param_spec_int("connection", "Connection", "Friend connection", TOX_CONNECTION_NONE, TOX_CONNECTION_UDP,
                         TOX_CONNECTION_NONE, param2);
    obj_properties[PROP_PUBLIC_KEY] =
        g_param_spec_string("public-key", "Public Key", "Friend public's key", NULL, param2);
    obj_properties[PROP_AVATAR_HASH] =
        g_param_spec_string("avatar-hash", "Avatar Hash", "Friend Avatar's hash", NULL, param2);

    gint param3 = G_PARAM_READWRITE;
    obj_properties[PROP_LAST_AVATAR_HASH_TRANSMITTED] =
        g_param_spec_string("last-avatar-hash-transmitted", "Last Avatar Hash Transmitted",
                            "The last transmitted avatar hash", NULL, param3);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_wrapperfriend_init(LupusWrapperFriend *instance) {}

LupusWrapperFriend *lupus_wrapperfriend_new(gpointer wrapper, guint friend_number)
{
    return g_object_new(LUPUS_TYPE_WRAPPERFRIEND, "wrapper", wrapper, "id", friend_number, NULL);
}