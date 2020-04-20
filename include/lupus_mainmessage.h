#ifndef __LUPUS_LUPUS_MAINMESSAGE_H__
#define __LUPUS_LUPUS_MAINMESSAGE_H__

#include "lupus_wrapperfriend.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINMESSAGE lupus_mainmessage_get_type()

G_DECLARE_FINAL_TYPE(LupusMainMessage, lupus_mainmessage, LUPUS, MAINMESSAGE, GtkEventBox)

/**
 * Create a new LupusMainMessage
 * @param friend LupusWrapperFriend or NULL if myself
 * @param message
 * @return a new instance of LupusMainMessage
 */
LupusMainMessage *lupus_mainmessage_new(LupusWrapperFriend *friend, gchar const *message);
gboolean lupus_mainmessage_is_myself(LupusMainMessage *instance);
void lupus_mainmessage_append(LupusMainMessage *instance, gchar const *message);

#endif