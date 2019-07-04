//
// Created by ogromny on 13/06/19.
//

#include "../include/lupus_headerbar.h"
#include "../include/lupus.h"

struct _LupusHeaderbar {
    GtkHeaderBar parent_instance;
};

G_DEFINE_TYPE(LupusHeaderbar, lupus_headerbar, GTK_TYPE_HEADER_BAR)

static void lupus_headerbar_init(LupusHeaderbar *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(instance), LUPUS_VERSION);
}

static void lupus_headerbar_class_init(LupusHeaderbarClass *class) {
    gchar *resource;

    resource = g_strconcat(LUPUS_RESOURCES, "/headerbar.ui", NULL);
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), resource);

    g_free(resource);
}

LupusHeaderbar *lupus_headerbar_new(void) {
    return g_object_new(LUPUS_TYPE_HEADERBAR, NULL);
}