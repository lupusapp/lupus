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

typedef enum {
    PROP_OBJECTSELF = 1,
    N_PROPERTIES,
} LupusObjectSaverProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

typedef enum {
    SET, // gboolean
    LAST_SIGNAL,
} LupusObjectSaverSignal;
static guint signals[LAST_SIGNAL];

gboolean save(LupusObjectSaver *instance)
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

static void set(LupusObjectSaver *instance, gboolean value)
{
    g_mutex_lock(&instance->mutex);
    instance->needed = value;
    g_mutex_unlock(&instance->mutex);
}

static gboolean check(LupusObjectSaver *instance)
{
    g_mutex_lock(&instance->mutex);

    if (instance->needed) {
        save(instance);
        instance->needed = FALSE;
    }

    g_mutex_unlock(&instance->mutex);

    return TRUE;
}

static void lupus_objectsaver_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusObjectSaver *instance = LUPUS_OBJECTSAVER(object);

    switch ((LupusObjectSaverProperty)property_id) {
    case PROP_OBJECTSELF:
        instance->objectself = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_objectsaver_finalize(GObject *object)
{
    LupusObjectSaver *instance = LUPUS_OBJECTSAVER(object);

    g_free(instance->filename);
    g_free(instance->password);

    g_source_remove(instance->event_source_id);
    g_mutex_unlock(&instance->mutex);
    g_mutex_clear(&instance->mutex);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_objectsaver_parent_class);
    parent_class->finalize(object);
}

static void lupus_objectsaver_constructed(GObject *object)
{
    LupusObjectSaver *instance = LUPUS_OBJECTSAVER(object);

    g_object_get(instance->objectself, "tox", &instance->tox, "profile-filename", &instance->filename,
                 "profile-password", &instance->password, NULL);

    g_mutex_init(&instance->mutex);
    instance->needed = FALSE;
    instance->event_source_id = g_timeout_add(5000, G_SOURCE_FUNC(check), instance);

    g_signal_connect(instance, "set", G_CALLBACK(set), NULL);

    GObjectClass *parent_class = G_OBJECT_CLASS(lupus_objectsaver_parent_class);
    parent_class->constructed(object);
}

static void lupus_objectsaver_class_init(LupusObjectSaverClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_objectsaver_constructed;
    object_class->finalize = lupus_objectsaver_finalize;
    object_class->set_property = lupus_objectsaver_set_property;

    obj_properties[PROP_OBJECTSELF] =
        g_param_spec_pointer("objectself", NULL, NULL, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    signals[SET] = g_signal_new("set", LUPUS_TYPE_OBJECTSAVER, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1,
                                G_TYPE_BOOLEAN);
}

static void lupus_objectsaver_init(LupusObjectSaver *instance) {}

LupusObjectSaver *lupus_objectsaver_new(LupusObjectSelf *objectself)
{
    return g_object_new(LUPUS_TYPE_OBJECTSAVER, "objectself", objectself, NULL);
}
