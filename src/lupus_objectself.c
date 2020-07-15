#include "include/lupus_objectself.h"
#include "glibconfig.h"
#include "include/lupus.h"
#include "include/lupus_objectfriend.h"
#include "include/lupus_objectsaver.h"
#include "include/lupus_objecttransfers.h"
#include "include/toxidenticon.h"
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <sodium/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tox/tox.h>
#include <tox/toxencryptsave.h>
#include <unistd.h>

/*! TODO: Remove all todo
 *  \todo Remove all todo
 */

/*! TODO: Refactor all switch
 *  \todo Refactor all switch
 */

/*! TODO: send remove-file-transfer (switch)
 *  \todo send remove-file-transfer (switch)
 */

struct _LupusObjectSelf {
    GObject parent_instance;

    Tox *tox;
    gchar *profile_filename;
    gchar *profile_password;

    LupusObjectTransfers *objecttransfers;
    GHashTable /*<friend_number: guint32, ObjectFriend>*/ *objectfriends;
    LupusObjectSaver *objectsaver;

    gchar *name;
    gchar *status_message;
    gchar *public_key;
    // make an object
    GdkPixbuf *avatar_pixbuf;
    gchar *avatar_hash;
    GByteArray *avatar_bytes; // cannot use avatar_pixbuf bytes, because it's not the actual data

    Tox_Connection connection;
    Tox_User_Status user_status;
    guint iterate_event_id;
    gchar *address; // TODO: emit when nospam/PK change
};

G_DEFINE_TYPE(LupusObjectSelf, lupus_objectself, G_TYPE_OBJECT)

#define t_n lupus_objectself
#define TN  LupusObjectSelf
#define T_N LUPUS_OBJECTSELF

declare_properties(PROP_TOX, PROP_PROFILE_FILENAME, PROP_PROFILE_PASSWORD, PROP_NAME, PROP_STATUS_MESSAGE,
                   PROP_PUBLIC_KEY, PROP_AVATAR_FILENAME, PROP_AVATAR_PIXBUF, PROP_AVATAR_HASH, PROP_CONNECTION,
                   PROP_USER_STATUS, PROP_ADDRESS, PROP_OBJECTFRIENDS, PROP_OBJECTSAVER);

declare_signals(FRIEND_REQUEST, // (gchar *public_key, gchar *message) -> gboolean
                FRIEND_ADDED,   // (guint32 friend_number)
                FRIEND_REMOVED, // (guint32 friend_number)
                ADD_FRIEND,     // (gchar *address_hex, gchar *message) -> gboolean
                REMOVE_FRIEND   // (guint32 friend_number) -> gboolean
);

static void objectfriends_remove_friend(INSTANCE, guint32 friend_number)
{
    gconstpointer key = GUINT_TO_POINTER(friend_number);
    LupusObjectFriend *objectfriend = LUPUS_OBJECTFRIEND(g_hash_table_lookup(instance->objectfriends, key));

    g_hash_table_remove(instance->objectfriends, GUINT_TO_POINTER(friend_number));
    g_object_notify_by_pspec(G_OBJECT(instance), obj_properties[PROP_OBJECTFRIENDS]);

    emit_by_pspec(instance, FRIEND_REMOVED, objectfriend);

    g_object_unref(objectfriend);
}

static void objectfriend_connection_status_cb(LupusObjectFriend *objectfriend, GParamSpec *pspec, INSTANCE)
{
    object_get_prop(objectfriend, "avatar-sent", avatar_sent, gboolean);
    if (avatar_sent) {
        return;
    }

    object_get_prop(objectfriend, "friend-number", friend_number, guint32);

    gsize avatar_filename_size = tox_public_key_size() * 2 + 4 + 1;
    gchar avatar_filename[avatar_filename_size];
    g_strlcat(avatar_filename, instance->public_key, avatar_filename_size);
    g_strlcat(avatar_filename, ".png", avatar_filename_size);

    FileTransfer *ft = file_transfer_new(TOX_FILE_KIND_AVATAR, instance->avatar_bytes->data,
                                         instance->avatar_bytes->len, g_strdup(avatar_filename), FALSE);

    Tox_Err_File_Send tox_err_file_send;
    guint32 file_number = tox_file_send(instance->tox, friend_number, ft->kind, ft->data_size, NULL,
                                        (guint8 *)ft->filename, strlen(ft->filename), &tox_err_file_send);

    switch (tox_err_file_send) {
    case TOX_ERR_FILE_SEND_OK:
        break;
    case TOX_ERR_FILE_SEND_NULL:
        lupus_error("Cannot send avatar, one argument is NULL.\nPlease open an issue on github.");
        return;
    case TOX_ERR_FILE_SEND_FRIEND_NOT_FOUND:
        return;
    case TOX_ERR_FILE_SEND_FRIEND_NOT_CONNECTED:
        return;
    case TOX_ERR_FILE_SEND_NAME_TOO_LONG:
        lupus_error("Cannot send avatar, filename is too long.\nPlease open an issue on github");
        return;
    case TOX_ERR_FILE_SEND_TOO_MANY:
        lupus_error("Cannot send avatar, maximum number of concurrent file transfers has been reached.");
        return;
    }

    emit_by_name(instance->objecttransfers, "create-file-transfer", friend_number, file_number, ft);
}

static void objectfriends_add_friend(INSTANCE, guint32 friend_number)
{
    LupusObjectFriend *objectfriend = lupus_objectfriend_new(instance, friend_number);

    g_hash_table_insert(instance->objectfriends, GUINT_TO_POINTER(friend_number), objectfriend);
    notify(instance, PROP_OBJECTFRIENDS);

    emit_by_pspec(instance, FRIEND_ADDED, objectfriend);

    connect(objectfriend, "notify::connection-status", objectfriend_connection_status_cb, instance);
}

static gboolean add_friend_cb(INSTANCE, gchar *address_hex, gchar *message)
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
        emit_by_name(instance->objectsaver, "set", TRUE);
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

static void objecttransfers_file_transfer_complete(INSTANCE, guint32 friend_number, guint32 file_number,
                                                   FileTransfer *ft)
{
    if (ft->receive_mode) {
        if (ft->kind == TOX_FILE_KIND_AVATAR) {
            GHashTable *objectfriends;
            g_object_get(instance, "objectfriends", &objectfriends, NULL);

            gpointer key = GUINT_TO_POINTER(friend_number);
            LupusObjectFriend *objectfriend = LUPUS_OBJECTFRIEND(g_hash_table_lookup(objectfriends, key));
            gchar *objectfriend_avatar_hash;
            g_object_get(objectfriend, "avatar-hash", &objectfriend_avatar_hash, NULL);

            guint8 avatar_hash[tox_hash_length()];
            tox_hash(avatar_hash, ft->data, ft->data_size);
            gchar avatar_hash_hex[tox_hash_length() * 2 + 1];
            sodium_bin2hex(avatar_hash_hex, sizeof(avatar_hash_hex), avatar_hash, sizeof(avatar_hash));

            if (g_strcmp0(objectfriend_avatar_hash, avatar_hash_hex)) {
                gchar *avatar_directory = g_strconcat(LUPUS_TOX_DIR, "avatars/", NULL);
                if (!g_file_test(avatar_directory, G_FILE_TEST_IS_DIR)) {
                    if (g_mkdir(avatar_directory, 755)) {
                        lupus_error("Cannot create avatars directory\n<b>%s</b>", avatar_directory);
                        g_free(avatar_directory);

                        return;
                    }
                }

                gchar public_key[tox_public_key_size() * 2 + 1];
                g_object_get(objectfriend, "public-key", &public_key, NULL);

                gsize filename_length = strlen(avatar_directory) + tox_public_key_size() * 2 + 4 + 1;
                gchar filename[filename_length];
                memset(filename, 0, filename_length);
                g_strlcat(filename, avatar_directory, filename_length);
                g_strlcat(filename, public_key, filename_length);
                g_strlcat(filename, ".png", filename_length);

                g_free(avatar_directory);

                GError *error = NULL;
                g_file_set_contents(filename, (gchar *)ft->data, ft->data_size, &error);
                if (error) {
                    lupus_error("Cannot save friend avatar: %s", error->message);
                    g_error_free(error);
                }

                emit_by_name(objectfriend, "refresh-avatar", NULL);
            }

            g_free(objectfriend_avatar_hash);
        }
    } else {
        if (ft->kind == TOX_FILE_KIND_AVATAR) {
            LupusObjectFriend *objectfriend =
                LUPUS_OBJECTFRIEND(g_hash_table_lookup(instance->objectfriends, GUINT_TO_POINTER(friend_number)));

            g_object_set(objectfriend, "avatar-sent", TRUE, NULL);
        }
    }

    emit_by_name(instance->objecttransfers, "remove-file-transfer", friend_number, file_number);
}

static gboolean remove_friend_cb(INSTANCE, guint friend_number)
{
    Tox_Err_Friend_Delete tox_err_friend_delete;
    tox_friend_delete(instance->tox, friend_number, &tox_err_friend_delete);

    switch (tox_err_friend_delete) {
    case TOX_ERR_FRIEND_DELETE_FRIEND_NOT_FOUND:
        lupus_error("Friend not found.");
        break;
    case TOX_ERR_FRIEND_DELETE_OK:
        objectfriends_remove_friend(instance, friend_number);
        emit_by_name(instance->objectsaver, "set", TRUE);
        return TRUE;
    }

    return FALSE;
}

static void connection_status_cb(Tox *tox, Tox_Connection connection_status, gpointer user_data)
{
    INSTANCE = LUPUS_OBJECTSELF(user_data);

    instance->connection = connection_status;

    GObject *object = G_OBJECT(instance);
    g_object_notify_by_pspec(object, obj_properties[PROP_CONNECTION]);
}

static void load_friends(INSTANCE)
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

static void load_avatar(INSTANCE, gchar *filename)
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
            guint8 *public_key = g_malloc(tox_public_key_size());
            tox_self_get_public_key(instance->tox, public_key);
            load_tox_identicon(public_key, instance->public_key, AVATAR_SIZE);
            g_free(public_key);

            load_avatar(instance, NULL);

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
    instance->avatar_hash = g_strdup(avatar_hash_hex);

    gchar *contents;
    gsize contents_length;
    g_file_get_contents(profile_avatar_filename, &contents, &contents_length, NULL);
    g_byte_array_remove_range(instance->avatar_bytes, 0, instance->avatar_bytes->len);
    g_byte_array_append(instance->avatar_bytes, (guint8 *)contents, contents_length);
    g_free(contents);

    GObject *object = G_OBJECT(instance);
    g_object_notify_by_pspec(object, obj_properties[PROP_AVATAR_PIXBUF]);
    g_object_notify_by_pspec(object, obj_properties[PROP_AVATAR_HASH]);
}

get_property_header()
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
case PROP_OBJECTSAVER:
    g_value_set_pointer(value, instance->objectsaver);
    break;
get_property_footer()

set_property_header()
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
    emit_by_name(instance->objectsaver, "set", TRUE);
    break;
case PROP_STATUS_MESSAGE:
    g_free(instance->status_message);
    instance->status_message = g_value_dup_string(value);
    tox_self_set_status_message(instance->tox, (guint8 *)instance->status_message, strlen(instance->status_message),
                                NULL);
    emit_by_name(instance->objectsaver, "set", TRUE);
    break;
case PROP_AVATAR_FILENAME:
    load_avatar(instance, (gchar *)g_value_get_string(value));
    break;
case PROP_USER_STATUS:
    instance->user_status = (Tox_User_Status)g_value_get_int(value);
    tox_self_set_status(instance->tox, instance->user_status);
    break;
set_property_footer()

static void lupus_objectself_finalize(GObject *object)
{
    INSTANCE = LUPUS_OBJECTSELF(object);

    g_free(instance->name);
    g_free(instance->status_message);
    g_free(instance->public_key);
    if (instance->avatar_pixbuf) {
        g_object_unref(instance->avatar_pixbuf);
    }
    g_free(instance->avatar_hash);
    g_byte_array_free(instance->avatar_bytes, TRUE);
    g_free(instance->address);

    tox_kill(instance->tox);
    g_free(instance->profile_filename);
    g_free(instance->profile_password);

    g_object_unref(instance->objecttransfers);
    g_hash_table_destroy(instance->objectfriends);
    g_object_unref(instance->objectsaver);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_objectself_parent_class);
    parent_class->finalize(object);
}

static gboolean iterate(INSTANCE)
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

static gchar *get_address(INSTANCE)
{
    guint8 bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(instance->tox, bin);

    gchar hex[TOX_ADDRESS_SIZE * 2 + 1];
    sodium_bin2hex(hex, sizeof(hex), bin, sizeof(bin));

    return g_ascii_strup(hex, -1);
}

static void file_recv_control_cb(Tox *tox, guint32 friend_number, guint32 file_number, TOX_FILE_CONTROL control,
                                 gpointer user_data)
{
    printf("file_recv_control: %d:%d %d\n", friend_number, file_number, control);
    fflush(stdout);
}

static void file_recv_cb(Tox *tox, guint32 friend_number, guint32 file_number, guint32 kind, guint64 file_size,
                         guchar const *filename, gsize filename_length, gpointer user_data)
{
    if (kind != TOX_FILE_KIND_AVATAR) {
        puts("file_recv_cb: non AVATAR data isn't supported yet");
        fflush(stdout);
        return;
    }

    INSTANCE = LUPUS_OBJECTSELF(user_data);

    FileTransfer *ft = file_transfer_new(kind, NULL, file_size, g_strndup((gchar *)filename, filename_length), TRUE);
    emit_by_name(instance->objecttransfers, "create-file-transfer", friend_number, file_number, ft);

    Tox_Err_File_Control tox_err_file_control;
    tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME, &tox_err_file_control);
    switch (tox_err_file_control) {
    case TOX_ERR_FILE_CONTROL_OK:
        break;
    case TOX_ERR_FILE_CONTROL_FRIEND_NOT_FOUND:
        lupus_error("Cannot accept file transfer, friend not found.");
        break;
    case TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED:
        lupus_error("Cannot accept file transfer, friend not connected.");
        break;
    case TOX_ERR_FILE_CONTROL_NOT_FOUND:
        lupus_error("This file transfer doesn't exist");
        break;
    case TOX_ERR_FILE_CONTROL_NOT_PAUSED:
        lupus_error("This file transfer is already running.");
        break;
    case TOX_ERR_FILE_CONTROL_DENIED:
        lupus_error("You cannot control this file transfer (you don't own it).");
        break;
    case TOX_ERR_FILE_CONTROL_ALREADY_PAUSED:
        lupus_error("This file transfer is already paused.");
        break;
    case TOX_ERR_FILE_CONTROL_SENDQ:
        lupus_error("The packet queue is full.");
        break;
    }
}

static void file_recv_chunk_cb(Tox *tox, guint32 friend_number, guint32 file_number, guint64 position,
                               guint8 const *data, gsize length, gpointer user_data)
{
    INSTANCE = LUPUS_OBJECTSELF(user_data);

    emit_by_name(instance->objecttransfers, "write-chunk", friend_number, file_number, position, data, length);
}

static void file_chunk_request_cb(Tox *tox, guint32 friend_number, guint32 file_number, guint64 position, gsize length,
                                  gpointer user_data)
{
    INSTANCE = LUPUS_OBJECTSELF(user_data);

    emit_by_name(instance->objecttransfers, "send-chunk", friend_number, file_number, position, length);
}

static void friend_request_cb(Tox *tox, guint8 const *public_key, guint8 const *message, gsize length,
                              gpointer user_data)
{
    INSTANCE = LUPUS_OBJECTSELF(user_data);

    gsize public_key_size = tox_public_key_size();
    gchar *message_request = g_strndup((gchar *)message, length);
    gchar *public_key_hex = g_malloc0(public_key_size * 2 + 1);
    sodium_bin2hex(public_key_hex, public_key_size * 2 + 1, public_key, public_key_size);

    gboolean accept = FALSE;
    emit_by_pspec(instance, FRIEND_REQUEST, public_key_hex, message_request, &accept);

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
            g_signal_emit_by_name(instance->objectsaver, "set", TRUE);
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

constructed_header()
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
    instance->avatar_bytes = g_byte_array_new();
    load_avatar(instance, NULL);

    instance->connection = TOX_CONNECTION_NONE;
    instance->user_status = TOX_USER_STATUS_NONE;

    instance->address = get_address(instance);

    instance->objecttransfers = lupus_objecttransfers_new(instance);

    instance->objectfriends = g_hash_table_new(NULL, NULL);
    load_friends(instance);

    instance->objectsaver = lupus_objectsaver_new(instance);

    tox_callback_self_connection_status(instance->tox, connection_status_cb);
    tox_callback_friend_request(instance->tox, friend_request_cb);
    tox_callback_file_recv_control(instance->tox, file_recv_control_cb);
    tox_callback_file_recv(instance->tox, file_recv_cb);
    tox_callback_file_recv_chunk(instance->tox, file_recv_chunk_cb);
    tox_callback_file_chunk_request(instance->tox, file_chunk_request_cb);

    bootstrap(instance->tox);
    g_timeout_add(tox_iteration_interval(instance->tox), G_SOURCE_FUNC(iterate), instance);

    connect(instance, "add-friend", add_friend_cb, NULL);
    connect(instance, "remove-friend", remove_friend_cb, NULL);
    connect_swapped(instance->objecttransfers, "file-transfer-complete", objecttransfers_file_transfer_complete,
                    instance);
constructed_footer()

class_init()
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_objectself_constructed;
    object_class->finalize = lupus_objectself_finalize;
    object_class->set_property = lupus_objectself_set_property;
    object_class->get_property = lupus_objectself_get_property;

    define_property(PROP_TOX, pointer, "tox", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    define_property(PROP_PROFILE_FILENAME, string, "profile-filename", NULL,
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    define_property(PROP_PROFILE_PASSWORD, string, "profile-password", NULL,
                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    define_property(PROP_NAME, string, "name", NULL, G_PARAM_READWRITE);
    define_property(PROP_STATUS_MESSAGE, string, "status-message", NULL, G_PARAM_READWRITE);
    define_property(PROP_PUBLIC_KEY, string, "public-key", NULL, G_PARAM_READABLE);
    define_property(PROP_AVATAR_FILENAME, string, "avatar-filename", NULL, G_PARAM_WRITABLE);
    define_property(PROP_AVATAR_PIXBUF, pointer, "avatar-pixbuf", G_PARAM_READABLE);
    define_property(PROP_AVATAR_HASH, string, "avatar-hash", NULL, G_PARAM_READABLE);
    define_property(PROP_CONNECTION, int, "connection", TOX_CONNECTION_NONE, TOX_CONNECTION_UDP, TOX_CONNECTION_NONE,
                    G_PARAM_READABLE);
    define_property(PROP_USER_STATUS, int, "user-status", TOX_USER_STATUS_NONE, TOX_USER_STATUS_BUSY,
                    TOX_USER_STATUS_NONE, G_PARAM_READWRITE);
    define_property(PROP_ADDRESS, string, "address", NULL, G_PARAM_READABLE);
    define_property(PROP_OBJECTFRIENDS, pointer, "objectfriends", G_PARAM_READABLE);
    define_property(PROP_OBJECTSAVER, pointer, "objectsaver", G_PARAM_READABLE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    define_signal(FRIEND_REQUEST, "friend-request", LUPUS_TYPE_OBJECTSELF, G_TYPE_BOOLEAN, 2, G_TYPE_STRING,
                  G_TYPE_STRING);
    define_signal(FRIEND_ADDED, "friend-added", LUPUS_TYPE_OBJECTSELF, G_TYPE_NONE, 1, LUPUS_TYPE_OBJECTFRIEND);
    define_signal(FRIEND_REMOVED, "friend-removed", LUPUS_TYPE_OBJECTSELF, G_TYPE_NONE, 1, LUPUS_TYPE_OBJECTFRIEND);
    define_signal(ADD_FRIEND, "add-friend", LUPUS_TYPE_OBJECTSELF, G_TYPE_BOOLEAN, 2, G_TYPE_STRING, G_TYPE_STRING);
    define_signal(REMOVE_FRIEND, "remove-friend", LUPUS_TYPE_OBJECTSELF, G_TYPE_BOOLEAN, 1, G_TYPE_UINT);
}

init() {}

LupusObjectSelf *lupus_objectself_new(Tox *tox, gchar *profile_filename, gchar *profile_password)
{
    return g_object_new(LUPUS_TYPE_OBJECTSELF, "tox", tox, "profile-filename", profile_filename, "profile-password",
                        profile_password, NULL);
}

