#ifndef __LUPUS_LUPUS_FRIEND_H__
#define __LUPUS_LUPUS_FRIEND_H__

#include "../include/lupus_objectfriend.h"
#include <glib-object.h>
#include <gtk/gtk.h>

#define LUPUS_TYPE_FRIEND lupus_friend_get_type()

G_DECLARE_FINAL_TYPE(LupusFriend, lupus_friend, LUPUS, FRIEND, GtkEventBox)

LupusFriend *lupus_friend_new(LupusObjectFriend *objectfriend);

#endif
