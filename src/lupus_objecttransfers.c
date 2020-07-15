#include "include/lupus_objecttransfers.h"
#include "glibconfig.h"
#include "include/lupus.h"
#include "include/lupus_objectself.h"
#include <stdio.h>
#include <string.h>
#include <tox/tox.h>

/*! TODO: handle all error and check for size (prevent overflow).
 *  \todo handle all error and check for size (prevent overflow).
 */

typedef GHashTable /*<friend_number, FriendTransfers>*/ Transfers;
typedef GHashTableIter TransfersIter;
typedef GHashTable /*<file_number, FileTransfer>*/ FriendTransfers;
typedef GHashTableIter FriendTransfersIter;

struct _LupusObjectTransfers {
    GObject parent_instance;

    Transfers *transfers;

    LupusObjectSelf *objectself;
};

G_DEFINE_TYPE(LupusObjectTransfers, lupus_objecttransfers, G_TYPE_OBJECT)

#define t_n lupus_objecttransfers
#define TN  LupusObjectTransfers
#define T_N LUPUS_OBJECTTRANSFERS

declare_properties(PROP_OBJECTSELF);

declare_signals(CREATE_FILE_TRANSFER,   // instance, friend_number, file_number, *ft
                REMOVE_FILE_TRANSFER,   // instance, friend_number, file_number
                WRITE_CHUNK,            // instance, friend_number, file_number, position, data, length
                FILE_TRANSFER_COMPLETE, // instance, friend_number, file_number, *ft
                SEND_CHUNK              // instance, friend_number, file_number, position, length
);

static void file_transfer_free(FileTransfer *file_transfer)
{
    g_free(file_transfer->filename);
    g_free(file_transfer->data);
    g_free(file_transfer);
}

FileTransfer *file_transfer_new(enum TOX_FILE_KIND kind, guint8 *data, gsize data_size, gchar *filename,
                                gboolean receive_mode)
{
    FileTransfer *ft = g_malloc0(sizeof(FileTransfer));
    ft->kind = kind;
    ft->data = g_malloc0(data_size);
    if (data) {
        memcpy(ft->data, data, data_size);
    }
    ft->data_size = data_size;
    ft->filename = filename;
    ft->receive_mode = receive_mode;
    return ft;
}

static void send_chunk(INSTANCE, guint32 friend_number, guint32 file_number, guint64 position, gsize length)
{
    FriendTransfers *friend_transfers = g_hash_table_lookup(instance->transfers, GUINT_TO_POINTER(friend_number));
    FileTransfer *ft = g_hash_table_lookup(friend_transfers, GUINT_TO_POINTER(file_number));

    object_get_prop(instance->objectself, "tox", tox, Tox *);

    if (length) {
        tox_file_send_chunk(tox, friend_number, file_number, position, ft->data + position, length, NULL);
        return;
    }

    tox_file_send_chunk(tox, friend_number, file_number, 0, NULL, 0, NULL);

    emit_by_pspec(instance, FILE_TRANSFER_COMPLETE, friend_number, file_number, ft);
}

static void write_chunk(INSTANCE, guint32 friend_number, guint32 file_number, guint64 position, guint8 const *data,
                        gsize length)
{
    FriendTransfers *friend_transfers = g_hash_table_lookup(instance->transfers, GUINT_TO_POINTER(friend_number));
    FileTransfer *ft = g_hash_table_lookup(friend_transfers, GUINT_TO_POINTER(file_number));

    if (length) {
        memcpy(ft->data + position, data, length);
        return;
    }

    object_get_prop(instance->objectself, "tox", tox, Tox *);
    tox_file_send_chunk(tox, friend_number, file_number, 0, NULL, 0, NULL);

    emit_by_pspec(instance, FILE_TRANSFER_COMPLETE, friend_number, file_number, ft);
}

static void remove_file_transfer(INSTANCE, guint32 friend_number, guint32 file_number)
{
    FriendTransfers *friend_transfers = g_hash_table_lookup(instance->transfers, GUINT_TO_POINTER(friend_number));
    if (!friend_transfers) {
        return;
    }

    gpointer file_number_key = GUINT_TO_POINTER(file_number);
    FileTransfer *file_transfer = g_hash_table_lookup(friend_transfers, file_number_key);
    if (!file_transfer) {
        return;
    }

    file_transfer_free(file_transfer);
    g_hash_table_remove(friend_transfers, file_number_key);
}

static void create_file_transfer(INSTANCE, guint32 friend_number, guint32 file_number, FileTransfer *ft)
{
    gpointer friend_number_key = GUINT_TO_POINTER(friend_number);
    FriendTransfers *friend_transfers = g_hash_table_lookup(instance->transfers, friend_number_key);
    if (!friend_transfers) {
        friend_transfers = g_hash_table_new(NULL, NULL);
        g_hash_table_insert(instance->transfers, friend_number_key, friend_transfers);
    }

    remove_file_transfer(instance, friend_number, file_number);
    g_hash_table_insert(friend_transfers, GUINT_TO_POINTER(file_number), ft);
}

set_property_header()
case PROP_OBJECTSELF:
    instance->objectself = g_value_get_pointer(value);
    break;
set_property_footer()

get_property_header()
case PROP_OBJECTSELF:
    g_value_set_pointer(value, instance->objectself);
    break;
get_property_footer()

finalize_header()
    TransfersIter transfers_iter;
    FriendTransfers *friend_transfers;
    g_hash_table_iter_init(&transfers_iter, instance->transfers);

    while (g_hash_table_iter_next(&transfers_iter, NULL, (gpointer *)&friend_transfers)) {
        FriendTransfersIter friend_transfers_iter;
        FileTransfer *file_transfer;
        g_hash_table_iter_init(&friend_transfers_iter, friend_transfers);

        while (g_hash_table_iter_next(&transfers_iter, NULL, (gpointer *)&file_transfer)) {
            file_transfer_free(file_transfer);
        }

        g_hash_table_destroy(friend_transfers);
    }

    g_hash_table_destroy(instance->transfers);
finalize_footer()

constructed_header()
    instance->transfers = g_hash_table_new(NULL, NULL);

    connect(instance, "create-file-transfer", create_file_transfer, NULL);
    connect(instance, "remove-file-transfer", remove_file_transfer, NULL);
    connect(instance, "write-chunk", write_chunk, NULL);
    connect(instance, "send-chunk", send_chunk, NULL);
constructed_footer()

class_init()
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_objecttransfers_constructed;
    object_class->finalize = lupus_objecttransfers_finalize;
    object_class->set_property = lupus_objecttransfers_set_property;
    object_class->get_property = lupus_objecttransfers_get_property;

    define_property(PROP_OBJECTSELF, pointer, "objectself", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

    define_signal(CREATE_FILE_TRANSFER, "create-file-transfer", LUPUS_TYPE_OBJECTTRANSFERS, G_TYPE_NONE, 3, G_TYPE_UINT,
                  G_TYPE_UINT, G_TYPE_POINTER);
    define_signal(REMOVE_FILE_TRANSFER, "remove-file-transfer", LUPUS_TYPE_OBJECTTRANSFERS, G_TYPE_NONE, 2, G_TYPE_UINT,
                  G_TYPE_UINT);
    define_signal(WRITE_CHUNK, "write-chunk", LUPUS_TYPE_OBJECTTRANSFERS, G_TYPE_NONE, 5, G_TYPE_UINT, G_TYPE_UINT,
                  G_TYPE_UINT64, G_TYPE_POINTER, G_TYPE_UINT);
    define_signal(FILE_TRANSFER_COMPLETE, "file-transfer-complete", LUPUS_TYPE_OBJECTTRANSFERS, G_TYPE_NONE, 3,
                  G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER);
    define_signal(SEND_CHUNK, "send-chunk", LUPUS_TYPE_OBJECTTRANSFERS, G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT,
                  G_TYPE_UINT64, G_TYPE_UINT);
}

init() {}

LupusObjectTransfers *lupus_objecttransfers_new(LupusObjectSelf *objectself)
{
    return g_object_new(LUPUS_TYPE_OBJECTTRANSFERS, "objectself", objectself, NULL);
}

