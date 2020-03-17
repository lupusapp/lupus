#ifndef __LUPUS_LUPUS_MAINCHAT_H__
#define __LUPUS_LUPUS_MAINCHAT_H__

#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINCHAT lupus_mainchat_get_type()

G_DECLARE_FINAL_TYPE(LupusMainChat, lupus_mainchat, LUPUS, MAINCHAT,
                     GtkEventBox)

LupusMainChat *lupus_mainchat_new(void);

#endif