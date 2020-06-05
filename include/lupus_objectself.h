#ifndef __LUPUS_LUPUS_OBJECTSELF_H__
#define __LUPUS_LUPUS_OBJECTSELF_H__

#include <glib-object.h>
#include <tox/tox.h>

#define LUPUS_TYPE_OBJECTSELF lupus_objectself_get_type()

G_DECLARE_FINAL_TYPE(LupusObjectSelf, lupus_objectself, LUPUS, OBJECTSELF, GObject)

LupusObjectSelf *lupus_objectself_new(Tox *tox, gchar *profile_filename, gchar *profile_password);

#endif