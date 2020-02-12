#ifndef __LUPUS_LUPUS_PROFILECHOOSER_H__
#define __LUPUS_LUPUS_PROFILECHOOSER_H__

#include "lupus_application.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_PROFILECHOOSER lupus_profilechooser_get_type()

G_DECLARE_FINAL_TYPE(LupusProfileChooser, lupus_profilechooser, LUPUS,
                     PROFILECHOOSER, GtkApplicationWindow)

LupusProfileChooser *lupus_profilechooser_new(LupusApplication *);

#endif