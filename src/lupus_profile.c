#include "../include/lupus_profile.h"
#include "../include/lupus.h"

struct _LupusProfile {
    GtkGrid parent_instance;
};

G_DEFINE_TYPE(LupusProfile, lupus_profile, GTK_TYPE_GRID)

static void lupus_profile_init(LupusProfile *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));
}

static void lupus_profile_class_init(LupusProfileClass *class) {
    gchar *resource;

    resource = g_strconcat(LUPUS_RESOURCES, "/profile.ui", NULL);
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), resource);

    g_free(resource);
}

LupusProfile *lupus_profile_new(void) {
    return g_object_new(LUPUS_TYPE_PROFILE, NULL);
}