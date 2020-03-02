#ifndef __LUPUS_LUPUS_MAINHEADERBAR_H__
#define __LUPUS_LUPUS_MAINHEADERBAR_H__

#include "lupus_main.h"
#include "toxcore/tox.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINHEADERBAR lupus_mainheaderbar_get_type()

G_DECLARE_FINAL_TYPE(LupusMainHeaderBar, lupus_mainheaderbar, LUPUS,
                     MAINHEADERBAR, GtkHeaderBar)

LupusMainHeaderBar *lupus_mainheaderbar_new(Tox const *, LupusMain const *,
                                            gint, gint);

#endif