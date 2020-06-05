#ifndef __LUPUS_LUPUS_PROFILE_H__
#define __LUPUS_LUPUS_PROFILE_H__

#include <gtk/gtk.h>
#include "../include/lupus_objectself.h"

#define LUPUS_TYPE_PROFILE lupus_profile_get_type()

G_DECLARE_FINAL_TYPE(LupusProfile, lupus_profile, LUPUS, PROFILE, GtkBox)

LupusProfile *lupus_profile_new(LupusObjectSelf *object_self);

#endif