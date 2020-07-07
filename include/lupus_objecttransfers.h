#ifndef __LUPUS_LUPUS_OBJECTTRANSFERS_H__
#define __LUPUS_LUPUS_OBJECTTRANSFERS_H__

#include "include/lupus_objectself.h"
#include <glib-object.h>

#define LUPUS_TYPE_OBJECTTRANSFERS lupus_objecttransfers_get_type()

G_DECLARE_FINAL_TYPE(LupusObjectTransfers, lupus_objecttransfers, LUPUS, OBJECTTRANSFERS, GObject)

typedef struct {
    enum TOX_FILE_KIND kind;
    guint64 data_size;
    guint8 *data;
    gchar *filename;
    gboolean receive_mode;
    /* TODO: support seeking <02-07-20, yourname> */
} FileTransfer;

FileTransfer *file_transfer_new(enum TOX_FILE_KIND kind, guint8 *data, gsize data_size, gchar *filename,
                                gboolean receive_mode);

LupusObjectTransfers *lupus_objecttransfers_new(LupusObjectSelf *objectself);

#endif
