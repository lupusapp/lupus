#include "../include/lupus_main.h"

struct _LupusMain {
    GtkApplicationWindow parent_instance;
};

typedef struct _LupusMainPrivate LupusMainPrivate;
struct _LupusMainPrivate {
    Tox *tox;
};

G_DEFINE_TYPE_WITH_PRIVATE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

enum {
    PROP_TOX = 1,
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
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_main_init(LupusMain *instance) {
    LupusHeaderbar *header_bar = lupus_headerbar_new();
    gtk_window_set_titlebar(GTK_WINDOW(instance), GTK_WIDGET(header_bar));

    LupusProfile *profile = lupus_profile_new();
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(profile), 0, 0, 1, 1);

    gtk_container_add(GTK_CONTAINER(instance), grid);

    gtk_widget_show_all(GTK_WIDGET(instance));
}

static void lupus_main_class_init(LupusMainClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = lupus_main_set_property;
    object_class->get_property = lupus_main_get_property;

    obj_properties[PROP_TOX] = g_param_spec_pointer(
            "tox",
            "Tox",
            "Tox instance to use.",
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE
    );

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

LupusMain *lupus_main_new(GtkApplication *application, Tox *tox) {
    return g_object_new(LUPUS_TYPE_MAIN,
                        "application", application,
                        "tox", tox,
                        NULL);
}