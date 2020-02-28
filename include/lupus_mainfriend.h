#ifndef __LUPUS_LUPUS_MAINFRIEND_H__
#define __LUPUS_LUPUS_MAINFRIEND_H__

#include "lupus_main.h"
#include "toxcore/tox.h"
#include <gtk/gtk.h>

#define LUPUS_TYPE_MAINFRIEND lupus_mainfriend_get_type()

G_DECLARE_FINAL_TYPE(LupusMainFriend, lupus_mainfriend, LUPUS, MAINFRIEND,
                     GtkEventBox)

enum {
    UPDATE_STATUS = 1 << 0,
    UPDATE_NAME = 1 << 1,
    UPDATE_STATUS_MESSAGE = 1 << 2,
    UPDATE_CONNECTION = 1 << 3
};

LupusMainFriend *lupus_mainfriend_new(Tox const *, LupusMain const *, guint32);

#endif