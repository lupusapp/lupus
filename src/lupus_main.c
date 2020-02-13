#include "../include/lupus_main.h"
#include "../include/lupus.h"

struct _LupusMain {
    GtkApplicationWindow parent_instance;

    Tox *tox;
    /*
     * data = {
     *  "profile filename",
     *  "profile password"
     * }
     */
    char const *profile_filename;
    char const *profile_password;

    GtkHeaderBar *header_bar;
};

G_DEFINE_TYPE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

enum {
    PROP_TOX = 1,
    PROP_PROFILE_FILENAME,
    PROP_PROFILE_PASSWORD,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void lupus_main_set_property(GObject *object, unsigned property_id,
                                    GValue const *value, GParamSpec *pspec) {
    LupusMain *instance = LUPUS_MAIN(object);
    switch (property_id) {
    case PROP_TOX:
        g_free(instance->tox); // TODO(ogromny): save tox before free
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_PROFILE_FILENAME:
        g_free((gpointer)instance->profile_filename);
        instance->profile_filename = g_value_get_string(value);
        break;
    case PROP_PROFILE_PASSWORD:
        g_free((gpointer)instance->profile_password);
        instance->profile_password = g_value_get_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_main_get_property(GObject *object, unsigned property_id,
                                    GValue *value, GParamSpec *pspec) {
    LupusMain *instance = LUPUS_MAIN(object);
    switch (property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, instance->tox);
        break;
    case PROP_PROFILE_FILENAME:
        g_value_set_string(value, instance->profile_filename);
        break;
    case PROP_PROFILE_PASSWORD:
        g_value_set_string(value, instance->profile_password);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_main_class_init(LupusMainClass *class) {
    G_OBJECT_CLASS(class)->set_property = lupus_main_set_property; // NOLINT
    G_OBJECT_CLASS(class)->get_property = lupus_main_get_property; // NOLINT

    /*
     * I don't want to use `constructed` so I handle manually `PROP_TOX` setter.
     * This is why I don't use `G_PARAM_CONSTRUCT_ONLY`.
     */
    obj_properties[PROP_TOX] =
        g_param_spec_pointer("tox", "Tox", "Tox profile.", G_PARAM_READWRITE);
    obj_properties[PROP_PROFILE_FILENAME] = g_param_spec_string(
        "profile-filename", "Profile filename", "Filename of the tox profile.",
        NULL, G_PARAM_READWRITE);
    obj_properties[PROP_PROFILE_PASSWORD] = g_param_spec_string(
        "profile-password", "Profile password", "Password of the tox profile.",
        NULL, G_PARAM_READWRITE);

    // NOLINTNEXTLINE
    g_object_class_install_properties(G_OBJECT_CLASS(class), N_PROPERTIES,
                                      obj_properties);
}

static void lupus_main_init(LupusMain *instance) {}

LupusMain *lupus_main_new(GtkApplication *application, Tox *tox,
                          char const *profile_filename,
                          char const *profile_password) {
    return g_object_new(LUPUS_TYPE_MAIN, "application", application, "tox", tox,
                        "profile-filename", profile_filename,
                        "profile-password", profile_password, NULL);
}