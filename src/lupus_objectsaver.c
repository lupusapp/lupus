#include "include/lupus_objectsaver.h"
#include "include/lupus.h"
#include "include/lupus_objectself.h"
#include <gtk/gtk.h>
#include <tox/toxencryptsave.h>

struct _LupusObjectSaver {
    GObject parent_instance;

    Tox *tox;
    gchar *filename;
    gchar *password;

    GMutex mutex;
    gboolean needed;
    guint event_source_id;

    LupusObjectSelf *objectself;
};

G_DEFINE_TYPE(LupusObjectSaver, lupus_objectsaver, G_TYPE_OBJECT)

#define t_n lupus_objectsaver
#define TN  LupusObjectSaver
#define T_N LUPUS_OBJECTSAVER

declare_properties(PROP_OBJECTSELF);
declare_signals(SET /* gboolean */);

gboolean save(INSTANCE)
{
    gsize savedata_size = tox_get_savedata_size(instance->tox);
    guint8 *savedata = g_malloc(savedata_size);
    tox_get_savedata(instance->tox, savedata);

    /* if password is set and is not empty */
    if (instance->password && *instance->password) {
        guint8 *tmp = g_malloc(savedata_size + tox_pass_encryption_extra_length());

        if (!tox_pass_encrypt(savedata, savedata_size, (guint8 *)instance->password, strlen(instance->password), tmp,
                              NULL)) {
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
    g_file_set_contents(instance->filename, (gchar *)savedata, savedata_size, &error);
    if (error) {
        lupus_error("Cannot save profile: %s", error->message);
        g_error_free(error);
        g_free(savedata);
        return FALSE;
    }

    g_free(savedata);
    return TRUE;
}

static void set(INSTANCE, gboolean value)
{
    g_mutex_lock(&instance->mutex);
    instance->needed = value;
    g_mutex_unlock(&instance->mutex);
}

static gboolean check(INSTANCE)
{
    g_mutex_lock(&instance->mutex);

    if (instance->needed) {
        save(instance);
        instance->needed = FALSE;
    }

    g_mutex_unlock(&instance->mutex);

    return TRUE;
}

set_property_header()
case PROP_OBJECTSELF:
    instance->objectself = g_value_get_pointer(value);
    break;
set_property_footer()

finalize_header()
    g_free(instance->filename);
    g_free(instance->password);

    g_source_remove(instance->event_source_id);
    g_mutex_unlock(&instance->mutex);
    g_mutex_clear(&instance->mutex);
finalize_footer()

constructed_header()
    g_object_get(instance->objectself, "tox", &instance->tox, "profile-filename", &instance->filename,
                 "profile-password", &instance->password, NULL);

    g_mutex_init(&instance->mutex);
    instance->needed = FALSE;
    instance->event_source_id = g_timeout_add(5000, G_SOURCE_FUNC(check), instance);

    connect(instance, "set", set, NULL);
constructed_footer()

class_init()
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_objectsaver_constructed;
    object_class->finalize = lupus_objectsaver_finalize;
    object_class->set_property = lupus_objectsaver_set_property;

    define_property(PROP_OBJECTSELF, pointer, "objectself", G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    define_signal(SET, "set", LUPUS_TYPE_OBJECTSAVER, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

init() {}

LupusObjectSaver *lupus_objectsaver_new(LupusObjectSelf *objectself)
{
    return g_object_new(LUPUS_TYPE_OBJECTSAVER, "objectself", objectself, NULL);
}
