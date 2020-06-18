#include "../include/lupus_friend.h"
#include <gtk/gtk.h>

struct _LupusFriend {
    GtkEventBox parent_instance;

    LupusObjectFriend *objectfriend;

    GtkBox *hbox;
    GtkImage *avatar;
    GtkBox *vbox;
    GtkLabel *name;
    GtkLabel *status_message;
};

G_DEFINE_TYPE(LupusFriend, lupus_friend, GTK_TYPE_EVENT_BOX)

typedef enum {
    PROP_OBJECTFRIEND = 1,
    N_PROPERTIES,
} LupusFriendProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void lupus_friend_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusFriend *instance = LUPUS_FRIEND(object);

    switch ((LupusFriendProperty)property_id) {
    case PROP_OBJECTFRIEND:
        instance->objectfriend = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_friend_constructed(GObject *object)
{
    LupusFriend *instance = LUPUS_FRIEND(object);

    gchar *name, *status_message;
    g_object_get(instance->objectfriend, "name", &name, "status-message", &status_message, NULL);
    instance->name = GTK_LABEL(g_object_new(GTK_TYPE_LABEL, "label", name, "halign", GTK_ALIGN_START, NULL));
    instance->status_message =
        GTK_LABEL(g_object_new(GTK_TYPE_LABEL, "label", status_message, "halign", GTK_ALIGN_START, NULL));

    instance->vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->name), FALSE, TRUE, 0);
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->status_message), TRUE, TRUE, 0);

    instance->hbox = GTK_BOX(g_object_new(GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_HORIZONTAL, "spacing", 5, NULL));
    instance->avatar = GTK_IMAGE(gtk_image_new_from_icon_name("utilities-terminal", GTK_ICON_SIZE_DND));
    gtk_box_pack_start(instance->hbox, GTK_WIDGET(instance->avatar), FALSE, TRUE, 0);
    gtk_box_pack_start(instance->hbox, GTK_WIDGET(instance->vbox), TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(instance), GTK_WIDGET(instance->hbox));

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_friend_parent_class);
    object_class->constructed(object);
}

static void lupus_friend_class_init(LupusFriendClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_friend_constructed;
    object_class->set_property = lupus_friend_set_property;

    obj_properties[PROP_OBJECTFRIEND] =
        g_param_spec_pointer("objectfriend", NULL, NULL, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_friend_init(LupusFriend *instance) {}

LupusFriend *lupus_friend_new(LupusObjectFriend *objectfriend)
{
    return g_object_new(LUPUS_TYPE_FRIEND, "objectfriend", objectfriend, NULL);
}
