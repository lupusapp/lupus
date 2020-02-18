#ifndef __LUPUS_LUPUS_MAINFRIENDLIST_H__
#define __LUPUS_LUPUS_MAINFRIENDLIST_H__

#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINFRIENDLIST lupus_mainfriendlist_get_type()

G_DECLARE_FINAL_TYPE(LupusMainFriendList, lupus_mainfriendlist, LUPUS, MAINFRIENDLIST, GtkEventBox)

LupusMainFriendList *lupus_mainfriendlist_new(void);

#endif