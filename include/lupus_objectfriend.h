#ifndef __LUPUS_LUPUS_OBJECTFRIEND_H__
#define __LUPUS_LUPUS_OBJECTFRIEND_H__

#include "include/lupus_objectself.h"
#include <glib-object.h>

#define LUPUS_TYPE_OBJECTFRIEND lupus_objectfriend_get_type()

G_DECLARE_FINAL_TYPE(LupusObjectFriend, lupus_objectfriend, LUPUS, OBJECTFRIEND, GObject)

LupusObjectFriend *lupus_objectfriend_new(LupusObjectSelf *objectself, guint32 friend_number);

#endif
