#ifndef __LUPUS_LUPUS_EDITABLELABEL_H__
#define __LUPUS_LUPUS_EDITABLELABEL_H__

#include "../include/lupus.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_EDITABLELABEL lupus_editablelabel_get_type()

G_DECLARE_FINAL_TYPE(LupusEditableLabel, lupus_editablelabel, LUPUS, EDITABLELABEL, GtkEventBox)

LupusEditableLabel *lupus_editablelabel_new(gchar *value, guint max_length);

#endif