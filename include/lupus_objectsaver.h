#ifndef __LUPUS_LUPUS_OBJECTSAVER_H__
#define __LUPUS_LUPUS_OBJECTSAVER_H__

#include "include/lupus.h"
#include "include/lupus_objectself.h"

#define LUPUS_TYPE_OBJECTSAVER lupus_objectsaver_get_type()

G_DECLARE_FINAL_TYPE(LupusObjectSaver, lupus_objectsaver, LUPUS, OBJECTSAVER, GObject)

LupusObjectSaver *lupus_objectsaver_new(LupusObjectSelf *objectself);

#endif
