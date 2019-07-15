#include "../include/lupus_main.h"
#include "../include/lupus_editable_label.h"
#include "../include/utils.h"

struct _LupusMain {
    GtkApplicationWindow parent_instance;
};

typedef struct _LupusMainPrivate LupusMainPrivate;
struct _LupusMainPrivate {
    Tox *tox;
    gchar *profile_name;
    gchar *profile_password;
};

G_DEFINE_TYPE_WITH_PRIVATE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

enum {
    PROP_TOX = 1,
    PROP_PROFILE_NAME,
    PROP_PROFILE_PASSWORD,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void lupus_main_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec) {
    LupusMainPrivate *priv = lupus_main_get_instance_private(LUPUS_MAIN(object));

    switch (property_id) {
        case PROP_TOX:
            //TODO: handle reset
            priv->tox = g_value_peek_pointer(value);
            break;
        case PROP_PROFILE_NAME:
            priv->profile_name = g_value_dup_string(value);
            break;
        case PROP_PROFILE_PASSWORD:
            priv->profile_password = g_value_dup_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_main_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
    LupusMainPrivate *priv = lupus_main_get_instance_private(LUPUS_MAIN(object));

    switch (property_id) {
        case PROP_TOX:
            g_value_set_pointer(value, priv->tox);
            break;
        case PROP_PROFILE_NAME:
            g_value_set_string(value, priv->profile_name);
            break;
        case PROP_PROFILE_PASSWORD:
            g_value_set_string(value, priv->profile_password);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void save_callback(LupusProfile *profile, gpointer user_data) {
    LupusMainPrivate *priv = lupus_main_get_instance_private(LUPUS_MAIN(user_data));
    save_tox(priv->tox, priv->profile_name, priv->profile_password, user_data);
}

static void lupus_main_constructed(GObject *object) {
    LupusMainPrivate *priv = lupus_main_get_instance_private(LUPUS_MAIN(object));

    LupusProfile *profile = lupus_profile_new(priv->tox);
    g_signal_connect(profile, "save", G_CALLBACK(save_callback), object);

    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(profile), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(separator), 0, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(object), grid);
    gtk_widget_show_all(grid);
}

static void lupus_main_init(LupusMain *instance) {
    gtk_window_set_titlebar(GTK_WINDOW(instance), GTK_WIDGET(lupus_headerbar_new()));
}

static void lupus_main_class_init(LupusMainClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->set_property = lupus_main_set_property;
    object_class->get_property = lupus_main_get_property;
    object_class->constructed = lupus_main_constructed;

    obj_properties[PROP_TOX] = g_param_spec_pointer(
            "tox",
            "Tox",
            "Tox instance to use.",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    obj_properties[PROP_PROFILE_NAME] = g_param_spec_string(
            "profile-filename",
            "profile_filename",
            "Profile's filename",
            "UNKNOWN",
            G_PARAM_READWRITE);
    obj_properties[PROP_PROFILE_PASSWORD] = g_param_spec_string(
            "profile-password",
            "profile_password",
            "Profile's password",
            "UNKNOWN",
            G_PARAM_READWRITE);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

LupusMain *lupus_main_new(GtkApplication *application, Tox *tox, gchar *profile_filename, gchar *profile_password) {
    return g_object_new(LUPUS_TYPE_MAIN,
                        "application", application,
                        "tox", tox,
                        "profile-filename", profile_filename,
                        "profile-password", profile_password,
                        NULL);
}