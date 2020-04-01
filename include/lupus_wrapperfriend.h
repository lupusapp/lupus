#ifndef __LUPUS_LUPUS_WRAPPERFRIEND_H__
#define __LUPUS_LUPUS_WRAPPERFRIEND_H__

#include "../include/lupus.h"
#include <glib-object.h>
#include <tox/tox.h>

#define LUPUS_TYPE_WRAPPERFRIEND lupus_wrapperfriend_get_type()

G_DECLARE_FINAL_TYPE(LupusWrapperFriend, lupus_wrapperfriend, LUPUS,
                     WRAPPERFRIEND, GObject)

LupusWrapperFriend *lupus_wrapperfriend_new(gpointer wrapper,
                                            guint friend_number);
void lupus_wrapperfriend_set_avatar_hash(LupusWrapperFriend *instance);

header_getter(wrapperfriend, WrapperFriend, id, guint);
header_getter(wrapperfriend, WrapperFriend, name, gchar *);
header_getter(wrapperfriend, WrapperFriend, status_message, gchar *);
header_getter(wrapperfriend, WrapperFriend, status, Tox_User_Status);
header_getter(wrapperfriend, WrapperFriend, connection, Tox_Connection);
header_getter(wrapperfriend, WrapperFriend, public_key, gchar *);
header_getter(wrapperfriend, WrapperFriend, avatar_hash, gchar *);
header_getter_setter(wrapperfriend, WrapperFriend, last_avatar_hash_transmitted,
                     gchar *);
#endif