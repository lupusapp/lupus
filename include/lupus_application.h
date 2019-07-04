#ifndef LUPUS_LUPUS_APPLICATION_H
#define LUPUS_LUPUS_APPLICATION_H

#include <gtk/gtk.h>

#define LUPUS_TYPE_APPLICATION lupus_application_get_type()

G_DECLARE_FINAL_TYPE(LupusApplication, lupus_application, LUPUS, APPLICATION, GtkApplication)

LupusApplication *lupus_application_new(void);

static void lupus_application_activate(GApplication *application);

#endif //LUPUS_LUPUS_APPLICATION_H
