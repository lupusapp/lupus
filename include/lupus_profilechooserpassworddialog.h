#ifndef __LUPUS_LUPUS_PROFILECHOOSERPASSWORDDIALOG_H__
#define __LUPUS_LUPUS_PROFILECHOOSERPASSWORDDIALOG_H__

#include "lupus_profilechooser.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_PROFILECHOOSERPASSWORDDIALOG                                \
    lupus_profilechooserpassworddialog_get_type()

G_DECLARE_FINAL_TYPE(LupusProfileChooserPasswordDialog,
                     lupus_profilechooserpassworddialog, LUPUS,
                     PROFILECHOOSERPASSWORDDIALOG, GtkDialog)

LupusProfileChooserPasswordDialog *lupus_profilechooserpassworddialog_new(void);

#endif