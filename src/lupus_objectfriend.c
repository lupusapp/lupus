#include "include/lupus_objectfriend.h"
#include "glibconfig.h"
#include "include/lupus.h"
#include "include/lupus_objectsaver.h"
#include "include/lupus_objectself.h"
#include "include/toxidenticon.h"
#include <gtk/gtk.h>
#include <sodium/utils.h>
#include <stdint.h>
#include <tox/tox.h>

struct _LupusObjectFriend {
    GObject parent_instance;

    LupusObjectSelf *objectself;
    guint32 friend_number;
    gchar *avatar_hash;
    GdkPixbuf *avatar_pixbuf;
    gboolean avatar_sent;

    Tox_Connection connection_status;
    Tox_User_Status status;
    gchar *name;
    gchar *status_message;
    gchar *public_key;
};

G_DEFINE_TYPE(LupusObjectFriend, lupus_objectfriend, G_TYPE_OBJECT)

#define t_n lupus_objectfriend
#define TN  LupusObjectFriend
#define T_N LUPUS_OBJECTFRIEND

declare_properties(PROP_OBJECTSELF, PROP_FRIEND_NUMBER, PROP_NAME, PROP_STATUS_MESSAGE, PROP_PUBLIC_KEY,
                   PROP_AVATAR_HASH, PROP_AVATAR_PIXBUF, PROP_AVATAR_SENT, PROP_CONNECTION_STATUS, PROP_STATUS);
declare_signals(REFRESH_AVATAR);

static LupusObjectFriend *get_instance_from_objectself(LupusObjectSelf *user_data, guint32 friend_number)
{
    object_get_prop(LUPUS_OBJECTSELF(user_data), "objectfriends", objectfriends, GHashTable *);
    return LUPUS_OBJECTFRIEND(g_hash_table_lookup(objectfriends, GUINT_TO_POINTER(friend_number)));
}

static void connection_status_cb(Tox *tox, guint32 friend_number, TOX_CONNECTION connection_status, gpointer user_data)
{
    INSTANCE = get_instance_from_objectself(user_data, friend_number);

    instance->connection_status = connection_status;

    notify(instance, PROP_CONNECTION_STATUS);
}

static void status_cb(Tox *tox, guint32 friend_number, TOX_USER_STATUS status, gpointer user_data)
{
    INSTANCE = get_instance_from_objectself(user_data, friend_number);

    instance->status = status;

    notify(instance, PROP_STATUS);
}

static void name_cb(Tox *tox, guint32 friend_number, guint8 const *name, gsize length, gpointer user_data)
{
    INSTANCE = get_instance_from_objectself(LUPUS_OBJECTSELF(user_data), friend_number);

    gchar *new_name = g_strndup((gchar *)name, length);
    if (!g_strcmp0(instance->name, new_name)) {
        free(new_name);
        return;
    }

    g_free(instance->name);
    instance->name = new_name;

    notify(instance, PROP_NAME);

    object_get_prop(instance->objectself, "objectsaver", objectsaver, LupusObjectSaver *);
    emit_by_name(objectsaver, "set", TRUE, NULL);
}

static void status_message_cb(Tox *tox, guint32 friend_number, guint8 const *status_message, gsize length,
                              gpointer user_data)
{
    INSTANCE = get_instance_from_objectself(LUPUS_OBJECTSELF(user_data), friend_number);

    gchar *new_status_message = g_strndup((gchar *)status_message, length);
    if (!g_strcmp0(instance->status_message, new_status_message)) {
        free(new_status_message);
        return;
    }

    g_free(instance->status_message);
    instance->status_message = new_status_message;

    notify(instance, PROP_STATUS_MESSAGE);

    object_get_prop(instance->objectself, "objectsaver", objectsaver, LupusObjectSaver *);
    emit_by_name(objectsaver, "set", TRUE, NULL);
}

static void load_avatar(INSTANCE)
{
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
        Tox *tox;
        g_object_get(instance->objectself, "tox", &tox, NULL);

        guint8 *public_key = g_malloc(tox_public_key_size());
        tox_friend_get_public_key(tox, instance->friend_number, public_key, NULL);
        load_tox_identicon(public_key, instance->public_key, AVATAR_FRIEND_SIZE);
        g_free(public_key);

        load_avatar(instance);

        return;
    }

    GError *error = NULL;
    instance->avatar_pixbuf =
        gdk_pixbuf_new_from_file_at_size(profile_avatar_filename, AVATAR_FRIEND_SIZE, AVATAR_FRIEND_SIZE, &error);
    if (error) {
        lupus_error("Cannot load friend's avatar %s\n<b>%s</b>", profile_avatar_filename, error->message);
        g_error_free(error);

        return;
    }

    gchar *contents;
    gsize length;
    g_file_get_contents(profile_avatar_filename, &contents, &length, NULL);

    guint8 avatar_hash_bin[tox_hash_length()];
    tox_hash(avatar_hash_bin, (guint8 *)contents, length);
    gchar avatar_hash_hex[tox_hash_length() * 2 + 1];
    sodium_bin2hex(avatar_hash_hex, sizeof(avatar_hash_hex), avatar_hash_bin, sizeof(avatar_hash_bin));
    instance->avatar_hash = g_strdup(avatar_hash_hex);

    g_free(contents);

    notify(instance, PROP_AVATAR_HASH);
    notify(instance, PROP_AVATAR_PIXBUF);
}

set_property_header()
case PROP_OBJECTSELF:
    instance->objectself = g_value_get_pointer(value);
    break;
case PROP_FRIEND_NUMBER:
    instance->friend_number = g_value_get_uint(value);
    break;
case PROP_AVATAR_SENT:
    instance->avatar_sent = g_value_get_boolean(value);
    break;
set_property_footer()

get_property_header()
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
case PROP_AVATAR_PIXBUF:
    g_value_set_pointer(value, instance->avatar_pixbuf);
    break;
case PROP_AVATAR_SENT:
    g_value_set_boolean(value, instance->avatar_sent);
    break;
case PROP_STATUS:
    g_value_set_int(value, instance->status);
    break;
case PROP_CONNECTION_STATUS:
    g_value_set_int(value, instance->connection_status);
    break;
get_property_footer()

finalize_header()
    g_free(instance->name);
    g_free(instance->status_message);
    g_free(instance->public_key);
    g_free(instance->avatar_hash);
    g_object_unref(instance->avatar_pixbuf);
finalize_footer()

constructed_header()
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

    instance->avatar_hash = NULL;
    load_avatar(instance);

    instance->avatar_sent = FALSE;

    g_signal_connect(instance, "refresh-avatar", G_CALLBACK(load_avatar), NULL);

    tox_callback_friend_connection_status(tox, connection_status_cb);
    tox_callback_friend_status(tox, status_cb);
    tox_callback_friend_name(tox, name_cb);
    tox_callback_friend_status_message(tox, status_message_cb);
constructed_footer()

class_init()
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_objectfriend_constructed;
    object_class->finalize = lupus_objectfriend_finalize;
    object_class->set_property = lupus_objectfriend_set_property;
    object_class->get_property = lupus_objectfriend_get_property;

    define_property(PROP_OBJECTSELF, pointer, "objectself", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    define_property(PROP_FRIEND_NUMBER, uint, "friend-number", 0, UINT32_MAX, 0,
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    define_property(PROP_NAME, string, "name", NULL, G_PARAM_READABLE);
    define_property(PROP_STATUS_MESSAGE, string, "status-message", NULL, G_PARAM_READABLE);
    define_property(PROP_PUBLIC_KEY, string, "public-key", NULL, G_PARAM_READABLE);
    define_property(PROP_AVATAR_HASH, string, "avatar-hash", NULL, G_PARAM_READABLE);
    define_property(PROP_AVATAR_PIXBUF, pointer, "avatar-pixbuf", G_PARAM_READABLE);
    define_property(PROP_AVATAR_SENT, boolean, "avatar-sent", FALSE, G_PARAM_READWRITE);
    define_property(PROP_CONNECTION_STATUS, int, "connection-status", TOX_CONNECTION_NONE, TOX_CONNECTION_UDP,
                    TOX_CONNECTION_NONE, G_PARAM_READABLE);
    define_property(PROP_STATUS, int, "status", TOX_USER_STATUS_NONE, TOX_USER_STATUS_BUSY, TOX_USER_STATUS_NONE,
                    G_PARAM_READABLE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    define_signal(REFRESH_AVATAR, "refresh-avatar", LUPUS_TYPE_OBJECTFRIEND, G_TYPE_NONE, 0, NULL);
}

init() {}

LupusObjectFriend *lupus_objectfriend_new(LupusObjectSelf *objectself, guint32 friend_number)
{
    return g_object_new(LUPUS_TYPE_OBJECTFRIEND, "objectself", objectself, "friend-number", friend_number, NULL);
}
