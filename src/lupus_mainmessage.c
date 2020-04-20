#include "../include/lupus_mainmessage.h"
#include "../include/lupus_wrapper.h"
#include <glib/ghash.h>

struct _LupusMainMessage {
    GtkEventBox parent_instance;

    LupusWrapperFriend *friend;
    gboolean myself;

    GtkBox *parent_box;
    GtkImage *image;
    GtkLabel *author, *message;
};

G_DEFINE_TYPE(LupusMainMessage, lupus_mainmessage, GTK_TYPE_EVENT_BOX);

#define PUBLIC_KEY                                                                                                     \
    ((instance->myself) ? lupus_wrapper_get_public_key(lupus_wrapper)                                                  \
                        : lupus_wrapperfriend_get_public_key(instance->friend))
#define AVATAR_SIZE 36

static GHashTable *avatars;
/*
 * GHashTable is an opaque type so I need to manually count ref
 */
static gint avatars_ref_count;

/* FIXME: notify_avatar_hash_cb */

typedef enum {
    PROP_FRIEND = 1,
    PROP_MESSAGE,
    N_PROPERTIES,
} LupusMainMessageProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

gboolean lupus_mainmessage_is_myself(LupusMainMessage *instance) { return instance->myself; }

void lupus_mainmessage_append(LupusMainMessage *instance, gchar const *message)
{
    gchar const *actual_message = gtk_label_get_text(instance->message);
    gchar *new_message = g_strconcat(actual_message, "\n", message, NULL);
    gtk_label_set_markup(instance->message, new_message);
    g_free(new_message);
}

static void set_author(LupusMainMessage *instance)
{
    gchar *author =
        instance->myself ? lupus_wrapper_get_name(lupus_wrapper) : lupus_wrapperfriend_get_name(instance->friend);
    gtk_label_set_text(instance->author, author);
}

static void set_image(LupusMainMessage *instance)
{
    gchar *public_key = PUBLIC_KEY;
    GdkPixbuf *pixbuf = g_hash_table_lookup(avatars, public_key);

    if (!pixbuf) {
        gchar *filename = g_strconcat(LUPUS_TOX_DIR, "avatars/", public_key, ".png", NULL);

        pixbuf = gdk_pixbuf_new_from_file_at_size(filename, AVATAR_SIZE, AVATAR_SIZE, NULL);
        g_hash_table_insert(avatars, public_key, pixbuf);

        g_free(filename);

        /*
         * Need to use a goto, otherwise pixbuf will get an unnecessary ref increment the first time
         */
        goto set;
    }

    g_object_ref(pixbuf);

set:
    gtk_image_set_from_pixbuf(instance->image, pixbuf);
}

static void lupus_mainmessage_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    LupusMainMessage *instance = LUPUS_MAINMESSAGE(object);

    switch ((LupusMainMessageProperty)property_id) {
    case PROP_FRIEND:
        instance->friend = g_value_get_pointer(value);
        instance->myself = !instance->friend;
        break;
    case PROP_MESSAGE:
        gtk_label_set_markup(instance->message, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_mainmessage_finalize(GObject *object)
{
    LupusMainMessage *instance = LUPUS_MAINMESSAGE(object);

    gpointer key = PUBLIC_KEY;
    GdkPixbuf *avatar = g_hash_table_lookup(avatars, key);
    if (avatar) {
        gint old_ref_count = G_OBJECT(avatar)->ref_count; // NOLINT

        g_object_unref(avatar);

        /*
         * Object has been freed
         */
        if (old_ref_count == 1) {
            g_hash_table_remove(avatars, key);
        }
    }

    g_hash_table_unref(avatars);
    if (--avatars_ref_count == 0) {
        avatars = NULL;
    }

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_mainmessage_parent_class); // NOLINT
    object_class->finalize(object);
}

static void lupus_mainmessage_constructed(GObject *object)
{
    LupusMainMessage *instance = LUPUS_MAINMESSAGE(object);

    set_author(instance);
    set_image(instance);

    if (instance->myself) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(instance->parent_box));
        gtk_box_reorder_child(instance->parent_box, children->data, 1);
        g_list_free(children);

        gtk_widget_set_halign(GTK_WIDGET(instance->author), GTK_ALIGN_END);
        gtk_widget_set_halign(GTK_WIDGET(instance->message), GTK_ALIGN_END);

        gtk_label_set_justify(instance->author, GTK_JUSTIFY_RIGHT);
        gtk_label_set_justify(instance->message, GTK_JUSTIFY_RIGHT);
    }

    gpointer obj = instance->myself ? (gpointer)lupus_wrapper : (gpointer)instance->friend;
    g_signal_connect_swapped(obj, "notify::name", G_CALLBACK(set_author), instance);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_mainmessage_parent_class); // NOLINT
    object_class->constructed(object);
}

static void lupus_mainmessage_class_init(LupusMainMessageClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass *object_class = G_OBJECT_CLASS(class); // NOLINT

    gtk_widget_class_set_template_from_resource(widget_class, LUPUS_RESOURCES "/mainmessage.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusMainMessage, parent_box);
    gtk_widget_class_bind_template_child(widget_class, LupusMainMessage, image);
    gtk_widget_class_bind_template_child(widget_class, LupusMainMessage, author);
    gtk_widget_class_bind_template_child(widget_class, LupusMainMessage, message);

    object_class->set_property = lupus_mainmessage_set_property;
    object_class->constructed = lupus_mainmessage_constructed;
    object_class->finalize = lupus_mainmessage_finalize;

    gint param = G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY; // NOLINT
    obj_properties[PROP_FRIEND] = g_param_spec_pointer("friend", "Friend", "LupusWrapperFriend of the friend", param);
    obj_properties[PROP_MESSAGE] = g_param_spec_string("message", "Message", "The message", NULL, param);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_mainmessage_init(LupusMainMessage *instance)
{
    gtk_widget_init_template(GTK_WIDGET(instance));

    if (!avatars) {
        avatars = g_hash_table_new(g_str_hash, g_str_equal);
        avatars_ref_count = 1;
    } else {
        g_hash_table_ref(avatars);
        ++avatars_ref_count;
    }
}

LupusMainMessage *lupus_mainmessage_new(LupusWrapperFriend *friend, gchar const *message)
{
    return g_object_new(LUPUS_TYPE_MAINMESSAGE, "friend", friend, "message", message, NULL);
}