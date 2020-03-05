#ifndef __LUPUS_LUPUS_MAINFRIENDLIST_H__
#define __LUPUS_LUPUS_MAINFRIENDLIST_H__

#include "lupus_main.h"
#include "lupus_mainfriend.h"
#include "toxcore/tox.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINFRIENDLIST lupus_mainfriendlist_get_type()

G_DECLARE_FINAL_TYPE(LupusMainFriendList, lupus_mainfriendlist, LUPUS,
                     MAINFRIENDLIST, GtkEventBox)

LupusMainFriendList *lupus_mainfriendlist_new(Tox const *, LupusMain *);

LupusMainFriend *get_friend(guint32);

#endif