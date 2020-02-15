#ifndef __LUPUS_LUPUS_APPLICATION_H__
#define __LUPUS_LUPUS_APPLICATION_H__

#include <gtk/gtk.h>

#define LUPUS_TYPE_APPLICATION lupus_application_get_type()

G_DECLARE_FINAL_TYPE(LupusApplication, lupus_application, LUPUS, APPLICATION,
                     GtkApplication)

LupusApplication *lupus_application_new(void);

#endif
