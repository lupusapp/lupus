#ifndef __LUPUS_LUPUS_WRAPPERFRIEND_H__
#define __LUPUS_LUPUS_WRAPPERFRIEND_H__

#include "../include/lupus.h"
#include "toxcore/tox.h"
#include <glib-object.h>

#define LUPUS_TYPE_WRAPPERFRIEND lupus_wrapperfriend_get_type()

G_DECLARE_FINAL_TYPE(LupusWrapperFriend, lupus_wrapperfriend, LUPUS,
                     WRAPPERFRIEND, GObject)

LupusWrapperFriend *lupus_wrapperfriend_new(gpointer wrapper,
                                            guint friend_number);

header_getter(wrapperfriend, WrapperFriend, id, guint);
header_getter(wrapperfriend, WrapperFriend, name, gchar *);
header_getter(wrapperfriend, WrapperFriend, status_message, gchar *);
header_getter(wrapperfriend, WrapperFriend, status, Tox_User_Status);
header_getter(wrapperfriend, WrapperFriend, connection, Tox_Connection);

#endif