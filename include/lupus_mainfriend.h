#ifndef __LUPUS_LUPUS_MAINFRIEND_H__
#define __LUPUS_LUPUS_MAINFRIEND_H__

#include "lupus_main.h"
#include "toxcore/tox.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINFRIEND lupus_mainfriend_get_type()

G_DECLARE_FINAL_TYPE(LupusMainFriend, lupus_mainfriend, LUPUS, MAINFRIEND,
                     GtkEventBox)

LupusMainFriend *lupus_mainfriend_new(guint32 friend_number);

#endif