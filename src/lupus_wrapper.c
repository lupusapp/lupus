#include "../include/lupus_wrapper.h"
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <sodium/utils.h>
#include <tox/toxencryptsave.h>

#define TOX_PORT 33445

struct _LupusWrapper {
    GObject parent_instance;

    guint listenning;
    guint32 active_chat_friend;
    gchar *avatar_hash;

    gchar *name;
    gchar *status_message;
    gchar *public_key;
    /* FIXME: emit notify::address when nospam or PK change */
    gchar *address;
    Tox_User_Status status;
    Tox_Connection connection;

    GHashTable *friends;

    Tox *tox;
    gchar *filename;
    gchar *password;
};

G_DEFINE_TYPE(LupusWrapper, lupus_wrapper, G_TYPE_OBJECT) // NOLINT

static GHashTable *files_out;
static GBytes *avatar_bytes;

typedef enum {
    PROP_TOX = 1,
    PROP_FILENAME,
    PROP_PASSWORD,
    PROP_NAME,
    PROP_STATUS_MESSAGE,
    PROP_PUBLIC_KEY,
    PROP_ADDRESS,
    PROP_STATUS,
    PROP_CONNECTION,
    PROP_FRIENDS,
    PROP_ACTIVE_CHAT_FRIEND,
    PROP_AVATAR_HASH,
    N_PROPERTIES,
} LupusWrapperProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

getter(wrapper, Wrapper, tox, Tox *);
getter_setter(wrapper, Wrapper, name, gchar *);
getter_setter(wrapper, Wrapper, status_message, gchar *);
getter(wrapper, Wrapper, public_key, gchar *);
getter(wrapper, Wrapper, address, gchar *);
getter_setter(wrapper, Wrapper, status, Tox_User_Status);
getter(wrapper, Wrapper, connection, Tox_Connection);
getter(wrapper, Wrapper, friends, GHashTable *);
getter_setter(wrapper, Wrapper, active_chat_friend, guint32);
getter(wrapper, Wrapper, avatar_hash, gchar *);

void lupus_wrapper_send_avatar(LupusWrapper *instance, guint32 friend_number) {
    static gchar *filename = "avatar";

    Tox_Err_File_Send tox_err_file_send = TOX_ERR_FILE_SEND_OK;
    gsize file_size = g_bytes_get_size(avatar_bytes);

    guint32 file_number =
        tox_file_send(instance->tox, friend_number, TOX_FILE_KIND_AVATAR,
                      file_size, (guint8 *)instance->avatar_hash,
                      (guint8 *)filename, strlen(filename), &tox_err_file_send);

    if (tox_err_file_send == TOX_ERR_FILE_SEND_OK) {
        g_hash_table_insert(files_out, GUINT_TO_POINTER(file_number),
                            g_bytes_ref(avatar_bytes));
    }
}

void lupus_wrapper_set_avatar_hash(LupusWrapper *instance) {
    gchar *filename = g_strconcat(LUPUS_TOX_DIR, "avatars/",
                                  instance->public_key, ".png", NULL);

    gsize contents_length = 0;
    gchar *contents = NULL;
    g_file_get_contents(filename, &contents, &contents_length, NULL);

    g_free(filename);

    guint8 hash[TOX_HASH_LENGTH];
    tox_hash(hash, (guint8 *)contents, contents_length);

    gchar hash_hex[TOX_HASH_LENGTH * 2 + 1];
    sodium_bin2hex(hash_hex, sizeof(hash_hex), hash, sizeof(hash));
    instance->avatar_hash = g_strdup(hash_hex);

    /*
     * Remove each file_number containing avatar_bytes.
     */
    if (avatar_bytes) {
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_hash_table_iter_init(&iter, files_out);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            if (value == avatar_bytes) {
                g_bytes_unref(avatar_bytes);
                g_hash_table_iter_remove(&iter);
            }
        }
        g_bytes_unref(avatar_bytes);
    }

    avatar_bytes = g_bytes_new(contents, contents_length);
    g_free(contents);

    /*
     * Call send_avatar foreach friends
     */
    GList *keys = g_hash_table_get_keys(instance->friends);
    for (GList *key = keys; key; key = key->next) {
        guint32 friend_number = GPOINTER_TO_UINT(key->data);
        lupus_wrapper_send_avatar(instance, friend_number);
    }
    g_list_free(keys);

    g_object_notify_by_pspec(G_OBJECT(instance), // NOLINT
                             obj_properties[PROP_AVATAR_HASH]);
}

void lupus_wrapper_add_friend(LupusWrapper *instance, guchar *address_bin,
                              guint8 *message, gsize message_size) {
    Tox_Err_Friend_Add tox_err_friend_add = TOX_ERR_FRIEND_ADD_OK;
    guint friend_number =
        (!message && !message_size)
            ? tox_friend_add_norequest(instance->tox, address_bin,
                                       &tox_err_friend_add)
            : tox_friend_add(instance->tox, address_bin, message, message_size,
                             &tox_err_friend_add);

    switch (tox_err_friend_add) {
    case TOX_ERR_FRIEND_ADD_OK:
        lupus_success("Friend successfully added.");
        break;
    case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
        lupus_error("Please enter a message.");
        return;
    case TOX_ERR_FRIEND_ADD_OWN_KEY:
        lupus_error("It's your personal address.");
        return;
    case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
        lupus_error("Friend request already sent.");
        return;
    case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
        lupus_error("Wrong friend address.");
        return;
    default:
        lupus_error("Cannot add friend.");
        return;
    }

    lupus_wrapper_save(instance);

    LupusWrapperFriend *friend =
        lupus_wrapperfriend_new(instance, friend_number);
    g_hash_table_insert(instance->friends, GUINT_TO_POINTER(friend_number),
                        friend);

    g_object_notify_by_pspec(G_OBJECT(instance), // NOLINT
                             obj_properties[PROP_FRIENDS]);
}

void lupus_wrapper_remove_friend(LupusWrapper *instance, guint friend_number) {
    gpointer key = GUINT_TO_POINTER(friend_number);
    LupusWrapperFriend *friend = g_hash_table_lookup(instance->friends, key);

    if (!friend) {
        return;
    }

    Tox_Err_Friend_Delete tox_err_friend_delete = TOX_ERR_FRIEND_DELETE_OK;
    tox_friend_delete(instance->tox, friend_number, &tox_err_friend_delete);
    if (tox_err_friend_delete != TOX_ERR_FRIEND_DELETE_OK) {
        lupus_error("Cannot delete friend.");
        return;
    }

    lupus_wrapper_save(instance);

    g_clear_object(&friend); // NOLINT
    g_hash_table_remove(instance->friends, key);
    g_object_notify_by_pspec(G_OBJECT(instance), // NOLINT
                             obj_properties[PROP_FRIENDS]);
}

LupusWrapperFriend *lupus_wrapper_get_friend(LupusWrapper *instance,
                                             guint friend_number) {
    return g_hash_table_lookup(instance->friends,
                               GUINT_TO_POINTER(friend_number));
}

static gboolean listening(LupusWrapper *instance) {
    tox_iterate(instance->tox, instance);
    return TRUE;
}

void lupus_wrapper_start_listening(LupusWrapper *instance) {
    if (instance->listenning != 0) {
        return;
    }
    instance->listenning = g_timeout_add(tox_iteration_interval(instance->tox),
                                         G_SOURCE_FUNC(listening), instance);
}

void lupus_wrapper_stop_listening(LupusWrapper *instance) {
    if (instance->listenning == 0) {
        return;
    }
    g_source_remove(instance->listenning);
}

gboolean lupus_wrapper_is_listening(LupusWrapper *instance) {
    return instance->listenning;
}

/* TODO: change bootstrap mechanics */
void lupus_wrapper_bootstrap(LupusWrapper *instance) {
    static struct {
        gchar *ip;
        guint16 port;
        gchar key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
        guchar key_bin[TOX_PUBLIC_KEY_SIZE];
    } nodes[] = {
        {"85.172.30.117", TOX_PORT,
         "8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832", 0},
        {"95.31.18.227", TOX_PORT,
         "257744DBF57BE3E117FE05D145B5F806089428D4DCE4E3D0D50616AA16D9417E", 0},
        {"94.45.70.19", TOX_PORT,
         "CE049A748EB31F0377F94427E8E3D219FC96509D4F9D16E181E956BC5B1C4564", 0},
        {"46.229.52.198", TOX_PORT,
         "813C8F4187833EF0655B10F7752141A352248462A567529A38B6BBF73E979307", 0},
    };

    for (gsize i = 0, j = G_N_ELEMENTS(nodes); i < j; ++i) {
        sodium_hex2bin(nodes[i].key_bin, sizeof(nodes[i].key_bin),
                       nodes[i].key_hex, sizeof(nodes[i].key_hex) - 1, NULL,
                       NULL, NULL);

        if (!tox_bootstrap(instance->tox, nodes[i].ip, nodes[i].port,
                           nodes[i].key_bin, NULL)) {
            g_warning("Cannot bootstrap %s.", nodes[i].ip);
        }
    }
}

gboolean lupus_wrapper_save(LupusWrapper *instance) {
    gsize savedata_size = tox_get_savedata_size(instance->tox);
    guint8 *savedata = g_malloc(savedata_size);
    tox_get_savedata(instance->tox, savedata);

    /* if password is set and is not empty */
    if (instance->password && *instance->password) {
        guint8 *tmp =
            g_malloc(savedata_size + TOX_PASS_ENCRYPTION_EXTRA_LENGTH);

        if (!tox_pass_encrypt(savedata, savedata_size,
                              (guint8 *)instance->password,
                              strlen(instance->password), tmp, NULL)) {
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
    g_file_set_contents(instance->filename, (gchar *)savedata, savedata_size,
                        &error);
    if (error) {
        lupus_error("Cannot save profile: %s", error->message);
        g_error_free(error);
        g_free(savedata);
        return FALSE;
    }

    g_free(savedata);
    return TRUE;
}

static void self_connection_status_cb(Tox *tox, // NOLINT
                                      TOX_CONNECTION connection_status,
                                      gpointer user_data) {
    LupusWrapper *instance = LUPUS_WRAPPER(user_data);
    GObject *gobject = G_OBJECT(user_data); // NOLINT

    instance->connection = connection_status;

    g_object_notify_by_pspec(gobject, obj_properties[PROP_CONNECTION]);
}

static void lupus_wrapper_get_property(GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec) {
    LupusWrapper *instance = LUPUS_WRAPPER(object);
    switch ((LupusWrapperProperty)property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, instance->tox);
        break;
    case PROP_FILENAME:
        g_value_set_string(value, instance->filename);
        break;
    case PROP_PASSWORD:
        g_value_set_string(value, instance->password);
        break;
    case PROP_NAME:
        g_value_set_string(value, lupus_wrapper_get_name(instance));
        break;
    case PROP_STATUS_MESSAGE:
        g_value_set_string(value, lupus_wrapper_get_status_message(instance));
        break;
    case PROP_PUBLIC_KEY:
        g_value_set_string(value, lupus_wrapper_get_public_key(instance));
        break;
    case PROP_ADDRESS:
        g_value_set_string(value, lupus_wrapper_get_address(instance));
        break;
    case PROP_STATUS:
        g_value_set_int(value, lupus_wrapper_get_status(instance));
        break;
    case PROP_CONNECTION:
        g_value_set_int(value, lupus_wrapper_get_connection(instance));
        break;
    case PROP_FRIENDS:
        g_value_set_pointer(value, lupus_wrapper_get_friends(instance));
        break;
    case PROP_ACTIVE_CHAT_FRIEND:
        g_value_set_uint(value, lupus_wrapper_get_active_chat_friend(instance));
        break;
    case PROP_AVATAR_HASH:
        g_value_set_string(value, lupus_wrapper_get_avatar_hash(instance));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

/* TODO: handle all return code */
static void lupus_wrapper_set_property(GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec) {
    LupusWrapper *instance = LUPUS_WRAPPER(object);
    switch ((LupusWrapperProperty)property_id) {
    case PROP_TOX:
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_FILENAME:
        instance->filename = g_value_dup_string(value);
        break;
    case PROP_PASSWORD:
        instance->password = g_value_dup_string(value);
        break;
    case PROP_NAME:
        instance->name = g_value_dup_string(value);
        tox_self_set_name(
            instance->tox, (guint8 *)instance->name,
            CLAMP(strlen(instance->name) + 1, 0, tox_max_name_length()), NULL);
        break;
    case PROP_STATUS_MESSAGE:
        instance->status_message = g_value_dup_string(value);
        tox_self_set_status_message(instance->tox,
                                    (guint8 *)instance->status_message,
                                    CLAMP(strlen(instance->status_message) + 1,
                                          0, tox_max_status_message_length()),
                                    NULL);
        break;
    case PROP_STATUS:
        instance->status = g_value_get_int(value);
        tox_self_set_status(instance->tox, instance->status);
        break;
    case PROP_ACTIVE_CHAT_FRIEND:
        instance->active_chat_friend = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void file_chunk_request_cb(Tox *tox, guint32 friend_number,
                                  guint32 file_number, guint64 position,
                                  gsize length, gpointer user_data) {
    gconstpointer key = GUINT_TO_POINTER(file_number);
    GBytes *bytes = g_hash_table_lookup(files_out, key);

    /*
     * Request terminated
     * set last_avatar_hash_transmitted to friend
     */
    if (!length) {
        LupusWrapper *instance = LUPUS_WRAPPER(user_data);
        LupusWrapperFriend *friend =
            lupus_wrapper_get_friend(instance, friend_number);

        guint8 file_id[TOX_FILE_ID_LENGTH];
        tox_file_get_file_id(tox, friend_number, file_number, file_id, NULL);

        if (g_strcmp0((gchar *)file_id, instance->avatar_hash) == 0) {
            lupus_wrapperfriend_set_last_avatar_hash_transmitted(
                friend, instance->avatar_hash);

            g_bytes_unref(bytes);
            return;
        }

        return;
    }

    if (!bytes) {
        tox_file_control(tox, friend_number, file_number,
                         TOX_FILE_CONTROL_CANCEL, NULL);
        return;
    }

    guint8 *data = (guint8 *)g_bytes_get_data(
        g_bytes_new_from_bytes(bytes, position, length), NULL);

    tox_file_send_chunk(tox, friend_number, file_number, position, data, length,
                        NULL);
}

static gboolean friends_destroy(gpointer key, gpointer value, // NOLINT
                                gpointer user_data) {         // NOLINT
    g_free(value);
    return TRUE;
}

static gboolean files_io_destroy(gpointer key, gpointer value, // NOLINT
                                 gpointer user_data) {         // NOLINT
    g_bytes_unref(value);
    return TRUE;
}

static void lupus_wrapper_finalize(GObject *object) {
    LupusWrapper *instance = LUPUS_WRAPPER(object);

    tox_kill(instance->tox);
    g_free(instance->filename);
    g_free(instance->password);
    g_free(instance->name);
    g_free(instance->status_message);
    g_free(instance->public_key);
    g_free(instance->address);
    g_free(instance->avatar_hash);

    g_hash_table_foreach_remove(instance->friends, friends_destroy, NULL);
    g_hash_table_foreach_remove(files_out, files_io_destroy, NULL);
    g_hash_table_destroy(instance->friends);
    g_hash_table_destroy(files_out);

    g_bytes_unref(avatar_bytes);
    // FIXME: empty files_io ?

    G_OBJECT_CLASS(lupus_wrapper_parent_class)->finalize(object); // NOLINT
}

static void lupus_wrapper_constructed(GObject *object) {
    LupusWrapper *instance = LUPUS_WRAPPER(object);

    instance->listenning = 0;

    gsize name_size = tox_self_get_name_size(instance->tox);
    if (name_size) {
        instance->name = g_malloc(name_size);
        tox_self_get_name(instance->tox, (guint8 *)instance->name);
        instance->name[name_size] = 0;
    } else {
        instance->name = NULL;
    }

    gsize status_message_size = tox_self_get_status_message_size(instance->tox);
    if (status_message_size) {
        instance->status_message = g_malloc(status_message_size);
        tox_self_get_status_message(instance->tox,
                                    (guint8 *)instance->status_message);
        instance->status_message[name_size] = 0;
    } else {
        instance->status_message = NULL;
    }

    guint8 public_key_bin[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_public_key(instance->tox, public_key_bin);
    gchar public_key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    sodium_bin2hex(public_key_hex, sizeof(public_key_hex), public_key_bin,
                   sizeof(public_key_bin));
    instance->public_key = g_ascii_strup(public_key_hex, -1);

    guint8 tox_address_bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(instance->tox, tox_address_bin);
    gchar tox_address_hex[TOX_ADDRESS_SIZE * 2 + 1];
    sodium_bin2hex(tox_address_hex, sizeof(tox_address_hex), tox_address_bin,
                   sizeof(tox_address_bin));
    instance->address = g_ascii_strup(tox_address_hex, -1);

    instance->status = tox_self_get_status(instance->tox);

    instance->connection = tox_self_get_connection_status(instance->tox);

    instance->friends = g_hash_table_new(NULL, NULL);
    gsize friend_list_size = tox_self_get_friend_list_size(instance->tox);
    if (friend_list_size) {
        guint32 friends[friend_list_size];
        tox_self_get_friend_list(instance->tox, friends);

        for (gsize i = 0; i < friend_list_size; ++i) {
            g_hash_table_insert(instance->friends, GUINT_TO_POINTER(friends[i]),
                                lupus_wrapperfriend_new(instance, friends[i]));
        }
    }

    lupus_wrapper_set_avatar_hash(instance);

    tox_callback_self_connection_status(instance->tox,
                                        self_connection_status_cb);
    tox_callback_file_chunk_request(instance->tox, file_chunk_request_cb);

    G_OBJECT_CLASS(lupus_wrapper_parent_class)->constructed(object); // NOLINT
}

static void lupus_wrapper_class_init(LupusWrapperClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS(class); // NOLINT
    object_class->constructed = lupus_wrapper_constructed;
    object_class->finalize = lupus_wrapper_finalize;
    object_class->set_property = lupus_wrapper_set_property;
    object_class->get_property = lupus_wrapper_get_property;

    gint param = G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY; // NOLINT
    obj_properties[PROP_TOX] =
        g_param_spec_pointer("tox", "Tox", "Tox instance", param);
    obj_properties[PROP_FILENAME] = g_param_spec_string(
        "filename", "Filename", "Profile filename", NULL, param);
    obj_properties[PROP_PASSWORD] = g_param_spec_string(
        "password", "Password", "Profile password", NULL, param);

    gint param2 = G_PARAM_READWRITE;
    obj_properties[PROP_NAME] =
        g_param_spec_string("name", "Name", "Profile name", NULL, param2);
    obj_properties[PROP_STATUS_MESSAGE] =
        g_param_spec_string("status-message", "Status message",
                            "Profile status message", NULL, param2);
    obj_properties[PROP_STATUS] = g_param_spec_int(
        "status", "Status", "Profile status", TOX_USER_STATUS_NONE,
        TOX_USER_STATUS_BUSY, TOX_USER_STATUS_NONE, param2);
    obj_properties[PROP_ACTIVE_CHAT_FRIEND] = g_param_spec_uint(
        "active-chat-friend", "Active chat friend",
        "Focused chat friend number", 0, UINT32_MAX, 0, param2);

    gint param3 = G_PARAM_READABLE;
    obj_properties[PROP_PUBLIC_KEY] = g_param_spec_string(
        "public-key", "Public Key", "Profile Public Key", NULL, param3);
    obj_properties[PROP_ADDRESS] = g_param_spec_string(
        "address", "Address", "Profile address", NULL, param3);
    obj_properties[PROP_CONNECTION] = g_param_spec_int(
        "connection", "Connection", "Profile connection status",
        TOX_CONNECTION_NONE, TOX_CONNECTION_UDP, TOX_CONNECTION_NONE, param3);
    obj_properties[PROP_FRIENDS] =
        g_param_spec_pointer("friends", "Friends", "Profile friends", param3);
    obj_properties[PROP_AVATAR_HASH] = g_param_spec_string(
        "avatar-hash", "Avatar Hash", "Profile Avatar Hash", NULL, param3);

    g_object_class_install_properties(object_class, N_PROPERTIES,
                                      obj_properties);
}

static void lupus_wrapper_init(LupusWrapper *instance) { // NOLINT
    files_out = g_hash_table_new(NULL, NULL);
    avatar_bytes = NULL;
}

LupusWrapper *lupus_wrapper_new(Tox *tox, gchar *filename, gchar *password) {
    return g_object_new(LUPUS_TYPE_WRAPPER, "tox", tox, "filename", filename,
                        "password", password, NULL);
}