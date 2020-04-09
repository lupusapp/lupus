#ifndef __LUPUS_LUPUS_MAINHEADERBAR_H__
#define __LUPUS_LUPUS_MAINHEADERBAR_H__

#include "lupus_main.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINHEADERBAR lupus_mainheaderbar_get_type()

G_DECLARE_FINAL_TYPE(LupusMainHeaderBar, lupus_mainheaderbar, LUPUS, MAINHEADERBAR, GtkBox)

LupusMainHeaderBar *lupus_mainheaderbar_new(void);
void lupus_mainheaderbar_reset_titles(LupusMainHeaderBar *instance);

#endif