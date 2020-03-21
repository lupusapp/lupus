#ifndef __LUPUS_LUPUS_MAINFRIEND_H__
#define __LUPUS_LUPUS_MAINFRIEND_H__

#include "../include/lupus.h"
#include "lupus_main.h"
#include <gtk/gtk.h>
#include <tox/tox.h>

#define LUPUS_TYPE_MAINFRIEND lupus_mainfriend_get_type()

G_DECLARE_FINAL_TYPE(LupusMainFriend, lupus_mainfriend, LUPUS, MAINFRIEND,
                     GtkEventBox)

LupusMainFriend *lupus_mainfriend_new(guint32 friend_number);
header_getter(mainfriend, MainFriend, friend_number, guint32);

#endif