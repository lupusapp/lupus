#include "../include/lupus_application.h"
#include "../include/lupus.h"
#include "../include/lupus_profilechooser.h"

struct _LupusApplication {
    GtkApplication parent_instance;
};

G_DEFINE_TYPE(LupusApplication, lupus_application, GTK_TYPE_APPLICATION)

static void lupus_application_activate(LupusApplication *application) {
    gtk_window_present(GTK_WINDOW(lupus_profilechooser_new(application)));
}

static void lupus_application_class_init(LupusApplicationClass *class) {
    G_APPLICATION_CLASS(class)->activate = lupus_application_activate; // NOLINT
}

static void lupus_application_init(LupusApplication *instance) {}

LupusApplication *lupus_application_new(void) {
    return g_object_new(LUPUS_TYPE_APPLICATION, "application-id",
                        LUPUS_APPLICATION_ID, NULL);
}