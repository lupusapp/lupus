#include "../include/lupus_application.h"
#include "../include/lupus_headerbar.h"
#include "../include/lupus_profile.h"
#include "../include/lupus_profile_chooser.h"
#include "../include/lupus.h"

struct _LupusApplication {
    GtkApplication parent_instance;
};

G_DEFINE_TYPE(LupusApplication, lupus_application, GTK_TYPE_APPLICATION)

static void lupus_application_init(LupusApplication *instance) {}

static void lupus_application_class_init(LupusApplicationClass *class) {
    G_APPLICATION_CLASS(class)->activate = lupus_application_activate;
}

static void lupus_application_activate(GApplication *application) {
    LupusProfileChooser *profile_chooser;

    profile_chooser = lupus_profile_chooser_new(LUPUS_APPLICATION(application));
    gtk_window_present(GTK_WINDOW(profile_chooser));
}

LupusApplication *lupus_application_new(void) {
    return g_object_new(LUPUS_TYPE_APPLICATION,
                        "application-id", LUPUS_APPLICATION_ID,
                        "flags", G_APPLICATION_FLAGS_NONE, // TODO: OPEN
                        NULL);
}