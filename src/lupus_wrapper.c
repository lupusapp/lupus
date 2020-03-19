#include "../include/lupus_wrapper.h"
#include "../include/lupus.h"
#include <gtk/gtk.h>
#include <sodium/utils.h>
#include <toxencryptsave/toxencryptsave.h>

#define TOX_PORT 33445

struct _LupusWrapper {
    GObject parent_instance;

    guint listenning;
    guint32 active_chat_friend;

    gchar *name;
    gchar *status_message;
    /* FIXME: emit notify::address when nospan or PK change */
    gchar address[TOX_ADDRESS_SIZE * 2 + 1];
    Tox_User_Status status;
    Tox_Connection connection;

    GHashTable *friends;

    Tox *tox;
    gchar *filename;
    gchar *password;
};

G_DEFINE_TYPE(LupusWrapper, lupus_wrapper, G_TYPE_OBJECT) // NOLINT

typedef enum {
    PROP_TOX = 1,
    PROP_FILENAME,
    PROP_PASSWORD,
    PROP_NAME,
    PROP_STATUS_MESSAGE,
    PROP_ADDRESS,
    PROP_STATUS,
    PROP_CONNECTION,
    PROP_FRIENDS,
    PROP_ACTIVE_CHAT_FRIEND,
    N_PROPERTIES,
} LupusWrapperProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

getter(wrapper, Wrapper, tox, Tox *);
getter_setter(wrapper, Wrapper, name, gchar *);
getter_setter(wrapper, Wrapper, status_message, gchar *);
getter(wrapper, Wrapper, address, gchar *);
getter_setter(wrapper, Wrapper, status, Tox_User_Status);
getter(wrapper, Wrapper, connection, Tox_Connection);
getter(wrapper, Wrapper, friends, GHashTable *);
getter_setter(wrapper, Wrapper, active_chat_friend, guint32);

void lupus_wrapper_add_friend(LupusWrapper *instance, guchar *address_bin,
                              guint8 *message, gsize message_size) {
    Tox_Err_Friend_Add tox_err_friend_add = TOX_ERR_FRIEND_ADD_OK;
    guint friend_number = tox_friend_add(instance->tox, address_bin, message,
                                         message_size, &tox_err_friend_add);

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
        g_value_set_string(value, lupus_wrapper_get_name(lupus_wrapper));
        break;
    case PROP_STATUS_MESSAGE:
        g_value_set_string(value,
                           lupus_wrapper_get_status_message(lupus_wrapper));
        break;
    case PROP_ADDRESS:
        g_value_set_string(value, lupus_wrapper_get_address(lupus_wrapper));
        break;
    case PROP_STATUS:
        g_value_set_int(value, lupus_wrapper_get_status(lupus_wrapper));
        break;
    case PROP_CONNECTION:
        g_value_set_int(value, lupus_wrapper_get_connection(lupus_wrapper));
        break;
    case PROP_FRIENDS:
        g_value_set_pointer(value, lupus_wrapper_get_friends(lupus_wrapper));
        break;
    case PROP_ACTIVE_CHAT_FRIEND:
        g_value_set_uint(value, instance->active_chat_friend);
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

static gboolean friends_destroy(gpointer key, gpointer value, // NOLINT
                                gpointer user_data) {         // NOLINT
    g_free(value);
    return TRUE;
}

static void lupus_wrapper_finalize(GObject *object) {
    LupusWrapper *instance = LUPUS_WRAPPER(object);

    tox_kill(instance->tox);
    g_free(instance->filename);
    g_free(instance->password);
    g_free(instance->name);
    g_free(instance->status_message);

    g_hash_table_foreach_remove(instance->friends, friends_destroy, NULL);
    g_hash_table_destroy(instance->friends);

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

    guint8 tox_address_bin[TOX_ADDRESS_SIZE];
    tox_self_get_address(instance->tox, tox_address_bin);
    sodium_bin2hex(instance->address, sizeof(instance->address),
                   tox_address_bin, sizeof(tox_address_bin));

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

    tox_callback_self_connection_status(instance->tox,
                                        self_connection_status_cb);

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
    obj_properties[PROP_ADDRESS] = g_param_spec_string(
        "address", "Address", "Profile address", NULL, param3);
    obj_properties[PROP_CONNECTION] = g_param_spec_int(
        "connection", "Connection", "Profile connection status",
        TOX_CONNECTION_NONE, TOX_CONNECTION_UDP, TOX_CONNECTION_NONE, param3);
    obj_properties[PROP_FRIENDS] =
        g_param_spec_pointer("friends", "Friends", "Profile friends", param3);

    g_object_class_install_properties(object_class, N_PROPERTIES,
                                      obj_properties);
}

static void lupus_wrapper_init(LupusWrapper *instance) {}

LupusWrapper *lupus_wrapper_new(Tox *tox, gchar *filename, gchar *password) {
    return g_object_new(LUPUS_TYPE_WRAPPER, "tox", tox, "filename", filename,
                        "password", password, NULL);
}