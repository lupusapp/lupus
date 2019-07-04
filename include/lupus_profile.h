#ifndef LUPUS_LUPUS_PROFILE_H
#define LUPUS_LUPUS_PROFILE_H

#include <gtk/gtk.h>

#define LUPUS_TYPE_PROFILE lupus_profile_get_type()

G_DECLARE_FINAL_TYPE(LupusProfile, lupus_profile, LUPUS, PROFILE, GtkGrid)

LupusProfile *lupus_profile_new(void);

#endif