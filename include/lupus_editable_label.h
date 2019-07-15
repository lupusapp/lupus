#ifndef LUPUS_LUPUS_EDITABLE_LABEL_H
#define LUPUS_LUPUS_EDITABLE_LABEL_H

#include <gtk/gtk.h>

#define LUPUS_TYPE_EDITABLE_LABEL lupus_editable_label_get_type()

G_DECLARE_FINAL_TYPE(LupusEditableLabel, lupus_editable_label, LUPUS, EDITABLE_LABEL, GtkEventBox)

LupusEditableLabel *lupus_editable_label_new(gchar *text);

#endif