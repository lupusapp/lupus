#include "../include/lupus_main.h"
#include "../include/lupus.h"
#include "../include/lupus_mainheaderbar.h"
#include "../include/utils.h"
#include <drm_mode.h>

struct _LupusMain {
    GtkApplicationWindow parent_instance;

    Tox const *tox;
    char const *profile_filename;
    char const *profile_password;

    LupusMainHeaderBar *main_header_bar;
};

G_DEFINE_TYPE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

enum {
    PROP_TOX = 1,
    PROP_PROFILE_FILENAME,
    PROP_PROFILE_PASSWORD,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

enum { SAVE, LAST_SIGNAL };

static unsigned signals[LAST_SIGNAL];

static void save_cb(LupusMain *instance) {
    tox_save((Tox *)instance->tox, instance->profile_filename,
             instance->profile_password, GTK_WINDOW(instance), false);
}

static void lupus_main_set_property(LupusMain *instance, unsigned property_id,
                                    GValue const *value, GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        g_free((gpointer)instance->tox); // TODO(ogromny): save tox before free
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_PROFILE_FILENAME:
        g_free((gpointer)instance->profile_filename);
        instance->profile_filename = g_value_dup_string(value);
        break;
    case PROP_PROFILE_PASSWORD:
        g_free((gpointer)instance->profile_password);
        instance->profile_password = g_value_dup_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_main_get_property(LupusMain *instance, unsigned property_id,
                                    GValue *value, GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, (gpointer)instance->tox);
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

static void lupus_main_constructed(LupusMain *instance) {
    gtk_window_set_titlebar(
        GTK_WINDOW(instance),
        GTK_WIDGET(instance->main_header_bar =
                       lupus_mainheaderbar_new(instance->tox, instance)));
}

static void lupus_main_class_init(LupusMainClass *class) {
    G_OBJECT_CLASS(class)->set_property = lupus_main_set_property; // NOLINT
    G_OBJECT_CLASS(class)->get_property = lupus_main_get_property; // NOLINT
    G_OBJECT_CLASS(class)->constructed = lupus_main_constructed;   // NOLINT

    obj_properties[PROP_TOX] = g_param_spec_pointer(
        "tox", "Tox", "Tox profile.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT
    obj_properties[PROP_PROFILE_FILENAME] = g_param_spec_string(
        "profile-filename", "Profile filename", "Filename of the tox profile.",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT
    obj_properties[PROP_PROFILE_PASSWORD] = g_param_spec_string(
        "profile-password", "Profile password", "Password of the tox profile.",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    // NOLINTNEXTLINE
    g_object_class_install_properties(G_OBJECT_CLASS(class), N_PROPERTIES,
                                      obj_properties);

    signals[SAVE] = g_signal_new("save", LUPUS_TYPE_MAIN, G_SIGNAL_RUN_LAST, 0,
                                 NULL, NULL, NULL, G_TYPE_NONE, 0); // NOLINT
}

static void lupus_main_init(LupusMain *instance) {
    g_signal_connect(instance, "save", G_CALLBACK(save_cb), NULL);
}

LupusMain *lupus_main_new(GtkApplication *application, Tox const *tox,
                          char const *profile_filename,
                          char const *profile_password) {
    return g_object_new(LUPUS_TYPE_MAIN, "application", application, "tox", tox,
                        "profile-filename", profile_filename,
                        "profile-password", profile_password, NULL);
}