#ifndef __LUPUS_LUPUS_OBJECTFRIEND_H__
#define __LUPUS_LUPUS_OBJECTFRIEND_H__

#include <glib-object.h>
#include <tox/tox.h>

#define LUPUS_TYPE_OBJECTFRIEND lupus_objectfriend_get_type()

G_DECLARE_FINAL_TYPE(LupusObjectFriend, lupus_objectfriend, LUPUS, OBJECTFRIEND, GObject)

LupusObjectFriend *lupus_objectfriend_new(Tox *tox, guint32 friend_number);

#endif
