#ifndef __LUPUS_LUPUS_WRAPPER_H__
#define __LUPUS_LUPUS_WRAPPER_H__

#include "../include/lupus.h"
#include "../include/lupus_wrapperfriend.h"
#include "../include/tox/tox.h"
#include <glib-object.h>

#define LUPUS_TYPE_WRAPPER lupus_wrapper_get_type()

G_DECLARE_FINAL_TYPE(LupusWrapper, lupus_wrapper, LUPUS, WRAPPER, GObject)

extern LupusWrapper *lupus_wrapper;

LupusWrapper *lupus_wrapper_new(Tox *tox, gchar *filename, gchar *password);

void lupus_wrapper_start_listening(LupusWrapper *instance);
void lupus_wrapper_stop_listening(LupusWrapper *instance);
gboolean lupus_wrapper_is_listening(LupusWrapper *instance);
gboolean lupus_wrapper_save(LupusWrapper *instance);
void lupus_wrapper_bootstrap(LupusWrapper *instance);
LupusWrapperFriend *lupus_wrapper_get_friend(LupusWrapper *instance,
                                             guint friend_number);
void lupus_wrapper_add_friend(LupusWrapper *instance, guchar *address_bin,
                              guint8 *message, gsize message_size);
void lupus_wrapper_remove_friend(LupusWrapper *instance, guint friend_number);

header_getter(wrapper, Wrapper, tox, Tox *);
header_getter_setter(wrapper, Wrapper, name, gchar *);
header_getter_setter(wrapper, Wrapper, status_message, gchar *);
header_getter(wrapper, Wrapper, address, gchar *);
header_getter_setter(wrapper, Wrapper, status, Tox_User_Status);
header_getter(wrapper, Wrapper, connection, Tox_Connection);
header_getter(wrapper, Wrapper, friends, GHashTable *);
header_getter_setter(wrapper, Wrapper, active_chat_friend, guint32);

#endif