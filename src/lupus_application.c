#include "../include/lupus_application.h"
#include "../include/lupus.h"
#include "../include/lupus_profilechooser.h"

struct _LupusApplication {
    GtkApplication parent_instance;
};

G_DEFINE_TYPE(LupusApplication, lupus_application, GTK_TYPE_APPLICATION)

#define t_n lupus_application
#define TN  LupusApplication
#define T_N LUPUS_APPLICATION

static void lupus_application_activate(GApplication *application)
{
    INSTANCE = LUPUS_APPLICATION(application);
    gtk_window_present(GTK_WINDOW(lupus_profilechooser_new(instance)));
}

class_init()
{
    GApplicationClass *application_class = G_APPLICATION_CLASS(class);
    application_class->activate = lupus_application_activate;
}

init() {}

LupusApplication *lupus_application_new(void)
{
    return g_object_new(LUPUS_TYPE_APPLICATION, "application-id", LUPUS_APPLICATION_ID, NULL);
}
