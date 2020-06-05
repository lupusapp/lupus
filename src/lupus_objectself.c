#include "../include/lupus_objectself.h"
#include "../include/lupus.h"
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <sodium/utils.h>
#include <tox/toxencryptsave.h>

struct _LupusObjectSelf {
    GObject parent_instance;

    Tox *tox;
    gchar *profile_filename;
    gchar *profile_password;

    gchar *name;
    gchar *status_message;
    gchar *public_key;
    GdkPixbuf *avatar_pixbuf;
    gchar *avatar_hash;
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
    N_PROPERTIES,
} LupusObjectSelfProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

#define AVATAR_SIZE 48
#define AVATAR_MAX_FILE_SIZE 65536

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

    tox_kill(instance->tox);
    g_free(instance->profile_filename);
    g_free(instance->profile_password);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_objectself_parent_class);
    parent_class->finalize(object);
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

    // TODO: listen tox

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

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_objectself_init(LupusObjectSelf *instance) {}

LupusObjectSelf *lupus_objectself_new(Tox *tox, gchar *profile_filename, gchar *profile_password)
{
    return g_object_new(LUPUS_TYPE_OBJECTSELF, "tox", tox, "profile-filename", profile_filename, "profile-password",
                        profile_password, NULL);
}