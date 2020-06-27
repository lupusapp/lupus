#include "../include/lupus_friend.h"
#include "glibconfig.h"
#include "include/lupus_objectfriend.h"
#include "include/lupus_objectself.h"
#include "pango/pango-layout.h"
#include <gtk/gtk.h>

struct _LupusFriend {
    GtkEventBox parent_instance;

    LupusObjectFriend *objectfriend;

    GtkBox *hbox;
    GtkImage *avatar;
    GtkBox *vbox;
    GtkLabel *name;
    GtkLabel *status_message;
    GtkMenu *popover;
};

G_DEFINE_TYPE(LupusFriend, lupus_friend, GTK_TYPE_EVENT_BOX)

typedef enum {
    PROP_OBJECTFRIEND = 1,
    N_PROPERTIES,
} LupusFriendProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void objectfriend_name_cb(gpointer user_data)
{
    LupusFriend *instance = LUPUS_FRIEND(user_data);

    gchar *name;
    g_object_get(instance->objectfriend, "name", &name, NULL);

    gtk_label_set_label(instance->name, name);
    gtk_label_set_ellipsize(instance->name, PANGO_ELLIPSIZE_END);
    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->name), name);
}

static void objectfriend_status_message_cb(gpointer user_data)
{
    LupusFriend *instance = LUPUS_FRIEND(user_data);

    gchar *status_message;
    g_object_get(instance->objectfriend, "status-message", &status_message, NULL);

    gtk_label_set_label(instance->status_message, status_message);
    gtk_label_set_ellipsize(instance->status_message, PANGO_ELLIPSIZE_END);
    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->status_message), status_message);
}

static gboolean button_press_event_cb(LupusFriend *instance, GdkEvent *event)
{
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        gtk_menu_popup_at_pointer(instance->popover, event);
    }

    return FALSE;
}

static void popover_remove_friend_cb(LupusFriend *instance)
{
    GtkDialog *dialog = GTK_DIALOG(g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, "border-width", 5, "title",
                                                "Are you sure ?", "resizable", FALSE, NULL));
    gtk_dialog_add_buttons(dialog, "Yes", GTK_RESPONSE_YES, "Cancel", GTK_RESPONSE_CANCEL, NULL);

    GtkBox *box = GTK_BOX(gtk_container_get_children(GTK_CONTAINER(dialog))->data);
    gtk_box_pack_start(box, gtk_label_new("Remove friend ?"), TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(dialog));

    if (gtk_dialog_run(dialog) == GTK_RESPONSE_YES) {
        guint32 friend_number;
        g_object_get(instance->objectfriend, "friend-number", &friend_number, NULL);

        LupusObjectSelf *objectself = NULL;
        g_object_get(instance->objectfriend, "objectself", &objectself, NULL);

        gboolean success;
        g_signal_emit_by_name(objectself, "remove-friend", friend_number, &success);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void construct_popover(LupusFriend *instance)
{
    instance->popover = GTK_MENU(gtk_menu_new());

    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_pack_start(box, gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON), FALSE, TRUE, 0);
    gtk_box_pack_start(box, gtk_label_new("Remove friend"), TRUE, TRUE, 0);

    GtkMenuItem *item = GTK_MENU_ITEM(gtk_menu_item_new());
    gtk_container_add(GTK_CONTAINER(item), GTK_WIDGET(box));

    gtk_menu_shell_append(GTK_MENU_SHELL(instance->popover), GTK_WIDGET(item));

    gtk_widget_show_all(GTK_WIDGET(instance->popover));

    g_signal_connect_swapped(item, "activate", G_CALLBACK(popover_remove_friend_cb), instance);
}

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

    instance->name = GTK_LABEL(g_object_new(GTK_TYPE_LABEL, "halign", GTK_ALIGN_START, NULL));
    instance->status_message = GTK_LABEL(g_object_new(GTK_TYPE_LABEL, "halign", GTK_ALIGN_START, NULL));
    objectfriend_name_cb(instance);
    objectfriend_status_message_cb(instance);

    instance->vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->name), FALSE, TRUE, 0);
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->status_message), TRUE, TRUE, 0);

    instance->hbox = GTK_BOX(g_object_new(GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_HORIZONTAL, "spacing", 5, NULL));
    instance->avatar = GTK_IMAGE(gtk_image_new_from_icon_name("utilities-terminal", GTK_ICON_SIZE_DND));
    gtk_box_pack_start(instance->hbox, GTK_WIDGET(instance->avatar), FALSE, TRUE, 0);
    gtk_box_pack_start(instance->hbox, GTK_WIDGET(instance->vbox), TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(instance), GTK_WIDGET(instance->hbox));

    construct_popover(instance);

    g_signal_connect_swapped(instance->objectfriend, "notify::name", G_CALLBACK(objectfriend_name_cb), instance);
    g_signal_connect_swapped(instance->objectfriend, "notify::status-message",
                             G_CALLBACK(objectfriend_status_message_cb), instance);
    g_signal_connect(instance, "button-press-event", G_CALLBACK(button_press_event_cb), NULL);

    LupusObjectSelf *objectself;
    g_object_get(instance->objectfriend, "objectself", &objectself, NULL);

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
