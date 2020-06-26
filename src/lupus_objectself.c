#include "../include/lupus_objectself.h"
#include "../include/lupus.h"
#include "../include/lupus_objectfriend.h"
#include "glibconfig.h"
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <sodium/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tox/tox.h>
#include <tox/toxencryptsave.h>

struct _LupusObjectSelf {
    GObject parent_instance;

    Tox *tox;
    gchar *profile_filename;
    gchar *profile_password;

    GHashTable /*<guint32, ObjectFriend>*/ *objectfriends;

    struct NeedSave {
        GMutex *mutex;
        gboolean value;
        guint event_source_id;
    } NeedSave;

    gchar *name;
    gchar *status_message;
    gchar *public_key;
    GdkPixbuf *avatar_pixbuf;
    gchar *avatar_hash;
    Tox_Connection connection;
    Tox_User_Status user_status;
    guint iterate_event_id;
    gchar *address; // TODO: emit when nospam/PK change
};

G_DEFINE_TYPE(LupusObjectSelf, lupus_objectself, G_TYPE_OBJECT)

typedef enum {
    PROP_TOX = 1,
    PROP_PROFILE_FILENAME,
    PROP_PROFILE_PASSWORD,
    PROP_NAME,
    PROP_STATUS_MESSAGE,
    PROP_PUBLIC_KEY,
    PROP_AVATAR_FILENAME,
    PROP_AVATAR_PIXBUF,
    PROP_AVATAR_HASH,
    PROP_CONNECTION,
    PROP_USER_STATUS,
    PROP_ADDRESS,
    PROP_OBJECTFRIENDS,
    N_PROPERTIES,
} LupusObjectSelfProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

typedef enum {
    SAVE,
    FRIEND_REQUEST, // (gchar *public_key, gchar *message) -> gboolean
    FRIEND_ADDED,   // (guint32 friend_number)
    FRIEND_REMOVED, // (guint32 friend_number)
    ADD_FRIEND,     // (gchar *address_hex, gchar *message) -> gboolean
    REMOVE_FRIEND,  // (guint32 friend_number) -> gboolean
    LAST_SIGNAL,
} LupusObjectSelfSignal;
static guint signals[LAST_SIGNAL];

#define AVATAR_MAX_FILE_SIZE 65536

gboolean save(LupusObjectSelf *instance)
{
    gsize savedata_size = tox_get_savedata_size(instance->tox);
    guint8 *savedata = g_malloc(savedata_size);
    tox_get_savedata(instance->tox, savedata);

    /* if password is set and is not empty */
    if (instance->profile_password && *instance->profile_password) {
        guint8 *tmp = g_malloc(savedata_size + tox_pass_encryption_extra_length());

        if (!tox_pass_encrypt(savedata, savedata_size, (guint8 *)instance->profile_password,
                              strlen(instance->profile_password), tmp, NULL)) {
            lupus_error("Cannot encrypt profile.");
            g_free(tmp);
            g_free(savedata);
            return FALSE;
        }

        g_free(savedata);
        savedata = tmp;
        savedata_size += TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
    }

    GError *error = NULL;
    g_file_set_contents(instance->profile_filename, (gchar *)savedata, savedata_size, &error);
    if (error) {
        lupus_error("Cannot save profile: %s", error->message);
        g_error_free(error);
        g_free(savedata);
        return FALSE;
    }

    g_free(savedata);
    return TRUE;
}

static void objectfriends_remove_friend(LupusObjectSelf *instance, guint32 friend_number)
{
    gconstpointer key = GUINT_TO_POINTER(friend_number);
    LupusObjectFriend *objectfriend = LUPUS_OBJECTFRIEND(g_hash_table_lookup(instance->objectfriends, key));

    g_hash_table_remove(instance->objectfriends, GUINT_TO_POINTER(friend_number));
    g_object_unref(objectfriend);

    g_object_notify_by_pspec(G_OBJECT(instance), obj_properties[PROP_OBJECTFRIENDS]);
    g_signal_emit(instance, signals[FRIEND_REMOVED], 0, friend_number);
}

static void objectfriends_add_friend(LupusObjectSelf *instance, guint32 friend_number)
{
    LupusObjectFriend *objectfriend = lupus_objectfriend_new(instance, friend_number);

    gpointer key = GUINT_TO_POINTER(friend_number);
    g_hash_table_insert(instance->objectfriends, key, objectfriend);

    g_object_notify_by_pspec(G_OBJECT(instance), obj_properties[PROP_OBJECTFRIENDS]);
    g_signal_emit(instance, signals[FRIEND_ADDED], 0, friend_number);
}

static gboolean add_friend_cb(LupusObjectSelf *instance, gchar *address_hex, gchar *message)
{
    guint8 address[tox_address_size()];
    sodium_hex2bin(address, sizeof(address), address_hex, tox_address_size() * 2, NULL, 0, NULL);

    Tox_Err_Friend_Add tox_err_friend_add = TOX_ERR_FRIEND_ADD_OK;
    guint32 friend_number =
        tox_friend_add(instance->tox, address, (guint8 *)message, strlen(message), &tox_err_friend_add);

    switch (tox_err_friend_add) {
    case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
        lupus_error("Request already sent.");
        break;
    case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
        lupus_error("Bad checksum.");
        break;
    case TOX_ERR_FRIEND_ADD_MALLOC:
        lupus_error("Allocation for friend request failed.");
        break;
    case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
        lupus_error("No message provied.");
        break;
    case TOX_ERR_FRIEND_ADD_NULL:
        lupus_error("Missing arguments, please open an issue at https://github.com/LupusApp/Lupus.");
        break;
    case TOX_ERR_FRIEND_ADD_OK:
        objectfriends_add_friend(instance, friend_number);
        save(instance);
        return TRUE;
    case TOX_ERR_FRIEND_ADD_OWN_KEY:
        lupus_error("It's your own public key.");
        break;
    case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
        lupus_error("The friend is already here, but the nospam value is different.");
        break;
    case TOX_ERR_FRIEND_ADD_TOO_LONG:
        lupus_error("Request message is too long.");
        break;
    }

    return FALSE;
}

static gboolean remove_friend_cb(LupusObjectSelf *instance, guint friend_number)
{
    Tox_Err_Friend_Delete tox_err_friend_delete;
    tox_friend_delete(instance->tox, friend_number, &tox_err_friend_delete);

    switch (tox_err_friend_delete) {
    case TOX_ERR_FRIEND_DELETE_FRIEND_NOT_FOUND:
        lupus_error("Friend not found.");
        break;
    case TOX_ERR_FRIEND_DELETE_OK:
        objectfriends_remove_friend(instance, friend_number);
        save(instance);
        return TRUE;
    }

    return FALSE;
}

static void need_save_set_cb(LupusObjectSelf *instance)
{
    g_mutex_lock(instance->NeedSave.mutex);
    instance->NeedSave.value = TRUE;
    g_mutex_unlock(instance->NeedSave.mutex);
}

static void connection_status_cb(Tox *tox, Tox_Connection connection_status, gpointer user_data)
{
    LupusObjectSelf *instance = LUPUS_OBJECTSELF(user_data);

    instance->connection = connection_status;

    GObject *object = G_OBJECT(instance);
    g_object_notify_by_pspec(object, obj_properties[PROP_CONNECTION]);
}

static void load_friends(LupusObjectSelf *instance)
{
    gsize friend_list_size = tox_self_get_friend_list_size(instance->tox);
    guint32 friend_list[friend_list_size];
    memset(friend_list, 0, friend_list_size);
    tox_self_get_friend_list(instance->tox, friend_list);

    for (gsize i = 0; i < friend_list_size; ++i) {
        guint32 friend_number = friend_list[i];
        objectfriends_add_friend(instance, friend_number);
    }
}

static void load_avatar(LupusObjectSelf *instance, gchar *filename)
{
    gchar *avatar_directory = g_strconcat(LUPUS_TOX_DIR, "avatars/", NULL);
    if (!g_file_test(avatar_directory, G_FILE_TEST_IS_DIR)) {
        if (g_mkdir(avatar_directory, 755)) {
            lupus_error("Cannot create avatars directory\n<b>%s</b>", avatar_directory);
            g_free(avatar_directory);

            return;
        }
    }

    gsize profile_avatar_filename_size = strlen(avatar_directory) + tox_public_key_size() * 2 + 4 + 1;
    gchar profile_avatar_filename[profile_avatar_filename_size];
    memset(profile_avatar_filename, 0, profile_avatar_filename_size);
    g_strlcat(profile_avatar_filename, avatar_directory, profile_avatar_filename_size);
    g_strlcat(profile_avatar_filename, instance->public_key, profile_avatar_filename_size);
    g_strlcat(profile_avatar_filename, ".png", profile_avatar_filename_size);
    g_free(avatar_directory);

    gchar *avatar_filename = filename;
    // No filename provided, so it's the first call (loading existing avatar).
    if (!avatar_filename) {
        avatar_filename = profile_avatar_filename;

        if (!g_file_test(avatar_filename, G_FILE_TEST_EXISTS)) {
            return;
        }
    }

    struct stat file_stat;
    if (!g_stat(filename, &file_stat) && file_stat.st_size > AVATAR_MAX_FILE_SIZE) {
        lupus_error("Avatar maximum size is <b>%d</b> bytes.", AVATAR_MAX_FILE_SIZE);

        return;
    }

    GError *error = NULL;
    instance->avatar_pixbuf = gdk_pixbuf_new_from_file_at_size(avatar_filename, AVATAR_SIZE, AVATAR_SIZE, &error);
    if (error) {
        lupus_error("Cannot load avatar %s\n<b>%s</b>", avatar_filename, error->message);
        g_error_free(error);

        return;
    }

    // New avatar provides, let's save it
    if (filename) {
        GError *error = NULL;
        if (!gdk_pixbuf_save(instance->avatar_pixbuf, profile_avatar_filename, "png", &error, NULL)) {
            lupus_error("Cannot save avatar.\n<b>%s</b>", error->message);
            g_error_free(error);

            return;
        }
    }

    guint8 const *avatar_data = gdk_pixbuf_read_pixels(instance->avatar_pixbuf);
    gsize avatar_data_length = gdk_pixbuf_get_byte_length(instance->avatar_pixbuf);
    guint8 avatar_hash_bin[tox_hash_length()];
    tox_hash(avatar_hash_bin, avatar_data, avatar_data_length);
    gchar avatar_hash_hex[tox_hash_length() * 2 + 1];
    sodium_bin2hex(avatar_hash_hex, sizeof(avatar_hash_hex), avatar_hash_bin, sizeof(avatar_hash_bin));
    instance->avatar_hash = g_ascii_strup(avatar_hash_hex, -1);

    GObject *object = G_OBJECT(instance);
    g_object_notify_by_pspec(object, obj_properties[PROP_AVATAR_PIXBUF]);
    g_object_notify_by_pspec(object, obj_properties[PROP_AVATAR_HASH]);
}

static gboolean need_save_check_cb(LupusObjectSelf *instance)
{
    g_mutex_lock(instance->NeedSave.mutex);

    if (instance->NeedSave.value) {
        save(instance);
        instance->NeedSave.value = FALSE;
    }

    g_mutex_unlock(instance->NeedSave.mutex);

    return TRUE;
}

static void lupus_objectself_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    LupusObjectSelf *instance = LUPUS_OBJECTSELF(object);

    switch ((LupusObjectSelfProperty)property_id) {
    case PROP_TOX:
        g_assert(instance->tox);
        g_value_set_pointer(value, instance->tox);
        break;
    case PROP_PROFILE_FILENAME:
        g_value_set_string(value, instance->profile_filename);
        break;
    case PROP_PROFILE_PASSWORD:
        g_value_set_string(value, instance->profile_password);
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
    case PROP_AVATAR_PIXBUF:
        g_value_set_pointer(value, instance->avatar_pixbuf);
        break;
    case PROP_AVATAR_HASH:
        g_value_set_string(value, instance->avatar_hash);
        break;
    case PROP_CONNECTION:
        g_value_set_int(value, instance->connection);
        break;
    case PROP_USER_STATUS:
        g_value_set_int(value, instance->user_status);
        break;
    case PROP_ADDRESS:
        g_value_set_string(value, instance->address);
        break;
    case PROP_OBJECTFRIENDS:
        g_value_set_pointer(value, instance->objectfriends);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_objectself_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusObjectSelf *instance = LUPUS_OBJECTSELF(object);

    switch ((LupusObjectSelfProperty)property_id) {
    case PROP_TOX:
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_PROFILE_FILENAME:
        instance->profile_filename = g_value_dup_string(value);
        break;
    case PROP_PROFILE_PASSWORD:
        instance->profile_password = g_value_dup_string(value);
        break;
    case PROP_NAME:
        g_free(instance->name);
        instance->name = g_value_dup_string(value);
        tox_self_set_name(instance->tox, (guint8 *)instance->name, strlen(instance->name), NULL);
        save(instance);
        break;
    case PROP_STATUS_MESSAGE:
        g_free(instance->status_message);
        instance->status_message = g_value_dup_string(value);
        tox_self_set_status_message(instance->tox, (guint8 *)instance->status_message, strlen(instance->status_message),
                                    NULL);
        save(instance);
        break;
    case PROP_AVATAR_FILENAME:
        load_avatar(instance, (gchar *)g_value_get_string(value));
        break;
    case PROP_USER_STATUS:
        instance->user_status = (Tox_User_Status)g_value_get_int(value);
        tox_self_set_status(instance->tox, instance->user_status);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_objectself_finalize(GObject *object)
{
    LupusObjectSelf *instance = LUPUS_OBJECTSELF(object);

    g_free(instance->name);
    g_free(instance->status_message);
    g_free(instance->public_key);
    if (instance->avatar_pixbuf) {
        g_object_unref(instance->avatar_pixbuf);
    }
    g_free(instance->avatar_hash);
    g_free(instance->address);

    tox_kill(instance->tox);
    g_free(instance->profile_filename);
    g_free(instance->profile_password);

    g_hash_table_destroy(instance->objectfriends);

    g_source_remove(instance->NeedSave.event_source_id);
    g_mutex_unlock(instance->NeedSave.mutex);
    g_mutex_clear(instance->NeedSave.mutex);
    g_free(instance->NeedSave.mutex);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_objectself_parent_class);
    parent_class->finalize(object);
}

static gboolean iterate(LupusObjectSelf *instance)
{
    tox_iterate(instance->tox, instance);
    return TRUE;
}

// TODO: bulletproof this
static void bootstrap(Tox *tox)
{
    typedef struct {
        gchar *ip;
        guint16 port;
        gchar key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        guchar key_bin[TOX_PUBLIC_KEY_SIZE];
    } Node;
    static Node nodes[] = {
        {"85.172.30.117", 33445, "8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832", {0}},
        {"95.31.18.227", 33445, "257744DBF57BE3E117FE05D145B5F806089428D4DCE4E3D0D50616AA16D9417E", {0}},
        {"94.45.70.19", 33445, "CE049A748EB31F0377F94427E8E3D219FC96509D4F9D16E181E956BC5B1C4564", {0}},
        {"46.229.52.198", 33445, "813C8F4187833EF0655B10F7752141A352248462A567529A38B6BBF73E979307", {0}},
    };

    for (gsize i = 0, j = G_N_ELEMENTS(nodes); i < j; ++i) {
        Node node = nodes[i];
        sodium_hex2bin(node.key_bin, sizeof(node.key_bin), node.key_hex, sizeof(node.key_hex) - 1, NULL, NULL, NULL);

        if (!tox_bootstrap(tox, node.ip, node.port, node.key_bin, NULL)) {
            g_warning("Cannot bootstrap %s.", node.ip);
        }
    }
}

static gchar *get_address(LupusObjectSelf *instance)
{
    guint8 bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(instance->tox, bin);

    gchar hex[TOX_ADDRESS_SIZE * 2 + 1];
    sodium_bin2hex(hex, sizeof(hex), bin, sizeof(bin));

    return g_ascii_strup(hex, -1);
}

static void friend_request_cb(Tox *tox, guint8 const *public_key, guint8 const *message, gsize length,
                              gpointer user_data)
{
    LupusObjectSelf *instance = LUPUS_OBJECTSELF(user_data);

    gsize public_key_size = tox_public_key_size();
    gchar *message_request = g_strndup((gchar *)message, length);
    gchar *public_key_hex = g_malloc0(public_key_size * 2 + 1);
    sodium_bin2hex(public_key_hex, public_key_size * 2 + 1, public_key, public_key_size);

    gboolean accept = FALSE;
    g_signal_emit(instance, signals[FRIEND_REQUEST], 0, public_key_hex, message_request, &accept);

    if (accept) {
        Tox_Err_Friend_Add tox_err_friend_add = TOX_ERR_FRIEND_ADD_OK;
        guint32 friend_number = tox_friend_add_norequest(tox, public_key, &tox_err_friend_add);
        switch (tox_err_friend_add) {
        case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
            lupus_error("Request already sent.");
            break;
        case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
            lupus_error("Bad checksum.");
            break;
        case TOX_ERR_FRIEND_ADD_MALLOC:
            lupus_error("Allocation for friend request failed.");
            break;
        case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
            lupus_error("No message provied.");
            break;
        case TOX_ERR_FRIEND_ADD_NULL:
            lupus_error("Missing arguments, please open an issue at https://github.com/LupusApp/Lupus.");
            break;
        case TOX_ERR_FRIEND_ADD_OK:
            objectfriends_add_friend(instance, friend_number);
            save(instance);
            break;
        case TOX_ERR_FRIEND_ADD_OWN_KEY:
            lupus_error("It's your own public key.");
            break;
        case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
            lupus_error("The friend is already here, but the nospam value is different.");
            break;
        case TOX_ERR_FRIEND_ADD_TOO_LONG:
            lupus_error("Request message is too long.");
            break;
        }
    }

    g_free(message_request);
    g_free(public_key_hex);
}

static void lupus_objectself_constructed(GObject *object)
{
    LupusObjectSelf *instance = LUPUS_OBJECTSELF(object);

    gsize name_size = tox_self_get_name_size(instance->tox);
    if (name_size) {
        instance->name = g_malloc0(name_size + 1);
        tox_self_get_name(instance->tox, (guint8 *)instance->name);
    } else {
        instance->name = NULL;
    }

    gsize status_message_size = tox_self_get_status_message_size(instance->tox);
    if (status_message_size) {
        instance->status_message = g_malloc0(status_message_size + 1);
        tox_self_get_status_message(instance->tox, (guint8 *)instance->status_message);
    } else {
        instance->status_message = NULL;
    }

    guint8 public_key_bin[tox_public_key_size()];
    tox_self_get_public_key(instance->tox, public_key_bin);
    gchar public_key_hex[tox_public_key_size() * 2 + 1];
    sodium_bin2hex(public_key_hex, sizeof(public_key_hex), public_key_bin, sizeof(public_key_bin));
    instance->public_key = g_ascii_strup(public_key_hex, -1);

    instance->avatar_hash = NULL;
    instance->avatar_pixbuf = NULL;
    load_avatar(instance, NULL);

    instance->connection = TOX_CONNECTION_NONE;
    instance->user_status = TOX_USER_STATUS_NONE;

    instance->address = get_address(instance);

    instance->objectfriends = g_hash_table_new(NULL, NULL);
    load_friends(instance);

    instance->NeedSave.mutex = g_malloc(sizeof(GMutex));
    g_mutex_init(instance->NeedSave.mutex);
    instance->NeedSave.value = FALSE;
    instance->NeedSave.event_source_id = g_timeout_add(5000, G_SOURCE_FUNC(need_save_check_cb), instance);

    tox_callback_self_connection_status(instance->tox, connection_status_cb);
    tox_callback_friend_request(instance->tox, friend_request_cb);

    bootstrap(instance->tox);
    g_timeout_add(tox_iteration_interval(instance->tox), G_SOURCE_FUNC(iterate), instance);

    g_signal_connect(instance, "save", G_CALLBACK(need_save_set_cb), NULL);
    g_signal_connect(instance, "add-friend", G_CALLBACK(add_friend_cb), NULL);
    g_signal_connect(instance, "remove-friend", G_CALLBACK(remove_friend_cb), NULL);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_objectself_parent_class);
    parent_class->constructed(object);
}

static void lupus_objectself_class_init(LupusObjectSelfClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_objectself_constructed;
    object_class->finalize = lupus_objectself_finalize;
    object_class->set_property = lupus_objectself_set_property;
    object_class->get_property = lupus_objectself_get_property;

    obj_properties[PROP_TOX] = g_param_spec_pointer("tox", NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_PROFILE_FILENAME] =
        g_param_spec_string("profile-filename", NULL, NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_PROFILE_PASSWORD] =
        g_param_spec_string("profile-password", NULL, NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_NAME] = g_param_spec_string("name", NULL, NULL, NULL, G_PARAM_READWRITE);
    obj_properties[PROP_STATUS_MESSAGE] = g_param_spec_string("status-message", NULL, NULL, NULL, G_PARAM_READWRITE);
    obj_properties[PROP_PUBLIC_KEY] = g_param_spec_string("public-key", NULL, NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_AVATAR_FILENAME] = g_param_spec_string("avatar-filename", NULL, NULL, NULL, G_PARAM_WRITABLE);
    obj_properties[PROP_AVATAR_PIXBUF] = g_param_spec_pointer("avatar-pixbuf", NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_AVATAR_HASH] = g_param_spec_string("avatar-hash", NULL, NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_CONNECTION] = g_param_spec_int("connection", NULL, NULL, TOX_CONNECTION_NONE,
                                                       TOX_CONNECTION_UDP, TOX_CONNECTION_NONE, G_PARAM_READABLE);
    obj_properties[PROP_USER_STATUS] = g_param_spec_int("user-status", NULL, NULL, TOX_USER_STATUS_NONE,
                                                        TOX_USER_STATUS_BUSY, TOX_USER_STATUS_NONE, G_PARAM_READWRITE);
    obj_properties[PROP_ADDRESS] = g_param_spec_string("address", NULL, NULL, NULL, G_PARAM_READABLE);
    obj_properties[PROP_OBJECTFRIENDS] = g_param_spec_pointer("objectfriends", NULL, NULL, G_PARAM_READABLE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    signals[FRIEND_REQUEST] = g_signal_new("friend-request", LUPUS_TYPE_OBJECTSELF, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                           NULL, G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_STRING);
    signals[FRIEND_ADDED] = g_signal_new("friend-added", LUPUS_TYPE_OBJECTSELF, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                         G_TYPE_NONE, 1, G_TYPE_UINT);
    signals[FRIEND_REMOVED] = g_signal_new("friend-removed", LUPUS_TYPE_OBJECTSELF, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                           NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
    signals[SAVE] = g_signal_new("save", LUPUS_TYPE_OBJECTSELF, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    signals[ADD_FRIEND] = g_signal_new("add-friend", LUPUS_TYPE_OBJECTSELF, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                       G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_STRING);
    signals[REMOVE_FRIEND] = g_signal_new("remove-friend", LUPUS_TYPE_OBJECTSELF, G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                          NULL, G_TYPE_BOOLEAN, 1, G_TYPE_UINT);
}

static void lupus_objectself_init(LupusObjectSelf *instance) {}

LupusObjectSelf *lupus_objectself_new(Tox *tox, gchar *profile_filename, gchar *profile_password)
{
    return g_object_new(LUPUS_TYPE_OBJECTSELF, "tox", tox, "profile-filename", profile_filename, "profile-password",
                        profile_password, NULL);
}

