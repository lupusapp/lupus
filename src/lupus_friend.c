#include "../include/lupus_friend.h"
#include "glibconfig.h"
#include "include/lupus.h"
#include "include/lupus_objectfriend.h"
#include "include/lupus_objectself.h"
#include "pango/pango-layout.h"
#include <gtk/gtk.h>
#include <tox/tox.h>

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

#define t_n lupus_friend
#define TN  LupusFriend
#define T_N LUPUS_FRIEND

declare_properties(PROP_OBJECTFRIEND);

static void objectfriend_name_cb(INSTANCE)
{
    gchar *name;
    g_object_get(instance->objectfriend, "name", &name, NULL);

    gtk_label_set_label(instance->name, name);
    gtk_label_set_ellipsize(instance->name, PANGO_ELLIPSIZE_END);
    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->name), name);

    g_free(name);
}

static void objectfriend_status_message_cb(INSTANCE)
{
    gchar *status_message;
    g_object_get(instance->objectfriend, "status-message", &status_message, NULL);

    gtk_label_set_label(instance->status_message, status_message);
    gtk_label_set_ellipsize(instance->status_message, PANGO_ELLIPSIZE_END);
    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->status_message), status_message);

    g_free(status_message);
}

static void objectfriend_avatar_pixbuf_cb(INSTANCE)
{
    GdkPixbuf *pixbuf;
    g_object_get(instance->objectfriend, "avatar-pixbuf", &pixbuf, NULL);

    gtk_image_set_from_pixbuf(instance->avatar, pixbuf);
}

static void objectfriend_status_cb(INSTANCE)
{
    Tox_User_Status status;
    g_object_get(instance->objectfriend, "status", &status, NULL);
    Tox_Connection connection_status;
    g_object_get(instance->objectfriend, "connection-status", &connection_status, NULL);

    if (connection_status == TOX_CONNECTION_NONE) {
        return;
    }

    widget_remove_classes_with_prefix(instance->avatar, "profile--");
    switch (status) {
    case TOX_USER_STATUS_NONE:
        widget_add_class(instance->avatar, "profile--none");
        break;
    case TOX_USER_STATUS_AWAY:
        widget_add_class(instance->avatar, "profile--away");
        break;
    case TOX_USER_STATUS_BUSY:
        widget_add_class(instance->avatar, "profile--busy");
        break;
    }
}

static void objectfriend_connection_status_cb(INSTANCE)
{
    Tox_Connection connection_status = TOX_CONNECTION_NONE;
    g_object_get(instance->objectfriend, "connection-status", &connection_status, NULL);

    if (connection_status != TOX_CONNECTION_NONE) {
        objectfriend_status_cb(instance);
        return;
    }

    widget_remove_classes_with_prefix(instance->avatar, "profile--");
}

static gboolean button_press_event_cb(INSTANCE, GdkEvent *event)
{
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        gtk_menu_popup_at_pointer(instance->popover, event);
    }

    return FALSE;
}

static void popover_public_key_cb(INSTANCE)
{
    static GtkWidget *dialog;

    if (!dialog) {
        dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, "title", "Friend's public key", "resizable",
                              FALSE, "border-width", 5, NULL);

        gchar *public_key = NULL;
        g_object_get(instance->objectfriend, "public-key", &public_key, NULL);
        GtkWidget *label = g_object_new(GTK_TYPE_LABEL, "label", public_key, "selectable", TRUE, NULL);
        g_free(public_key);

        GtkBox *box = GTK_BOX(gtk_bin_get_child(GTK_BIN(dialog)));
        gtk_box_pack_start(box, label, TRUE, TRUE, 0);

        gtk_widget_show_all(dialog);
    }

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
}

static void popover_remove_friend_cb(INSTANCE)
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
        emit_by_name(objectself, "remove-friend", friend_number, &success);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void construct_popover(INSTANCE)
{
    instance->popover = GTK_MENU(gtk_menu_new());

    GtkBox *remove_friend_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_pack_start(remove_friend_box, gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON), FALSE,
                       TRUE, 0);
    gtk_box_pack_start(remove_friend_box, gtk_label_new("Remove friend"), TRUE, TRUE, 0);
    GtkMenuItem *remove_friend_item = GTK_MENU_ITEM(gtk_menu_item_new());
    gtk_container_add(GTK_CONTAINER(remove_friend_item), GTK_WIDGET(remove_friend_box));

    GtkBox *public_key_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_pack_start(public_key_box, gtk_image_new_from_resource(LUPUS_RESOURCES "/biometric.svg"), FALSE, TRUE, 0);
    gtk_box_pack_start(public_key_box, gtk_label_new("Public key"), TRUE, TRUE, 0);
    GtkMenuItem *public_key_item = GTK_MENU_ITEM(gtk_menu_item_new());
    gtk_container_add(GTK_CONTAINER(public_key_item), GTK_WIDGET(public_key_box));

    gtk_menu_shell_append(GTK_MENU_SHELL(instance->popover), GTK_WIDGET(remove_friend_item));
    gtk_menu_shell_append(GTK_MENU_SHELL(instance->popover), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(instance->popover), GTK_WIDGET(public_key_item));

    gtk_widget_show_all(GTK_WIDGET(instance->popover));

    connect_swapped(remove_friend_item, "activate", popover_remove_friend_cb, instance);
    connect_swapped(public_key_item, "activate", popover_public_key_cb, instance);
}

set_property_header()
case PROP_OBJECTFRIEND:
    instance->objectfriend = g_value_get_pointer(value);
    break;
set_property_footer()

constructed_header()
    instance->name = GTK_LABEL(g_object_new(GTK_TYPE_LABEL, "halign", GTK_ALIGN_START, NULL));
    objectfriend_name_cb(instance);
    instance->status_message = GTK_LABEL(g_object_new(GTK_TYPE_LABEL, "halign", GTK_ALIGN_START, NULL));
    objectfriend_status_message_cb(instance);

    instance->vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->name), FALSE, TRUE, 0);
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->status_message), TRUE, TRUE, 0);

    object_get_prop(instance->objectfriend, "avatar-pixbuf", avatar_pixbuf, GdkPixbuf *);
    instance->avatar = GTK_IMAGE(gtk_image_new_from_pixbuf(avatar_pixbuf));
    widget_add_class(instance->avatar, "profile");

    instance->hbox = GTK_BOX(g_object_new(GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_HORIZONTAL, "spacing", 5, NULL));
    gtk_box_pack_start(instance->hbox, GTK_WIDGET(instance->avatar), FALSE, TRUE, 0);
    gtk_box_pack_start(instance->hbox, GTK_WIDGET(instance->vbox), TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(instance), GTK_WIDGET(instance->hbox));

    construct_popover(instance);

    connect_swapped(instance->objectfriend, "notify::name", objectfriend_name_cb, instance);
    connect_swapped(instance->objectfriend, "notify::status-message", objectfriend_status_message_cb, instance);
    connect_swapped(instance->objectfriend, "notify::avatar-pixbuf", objectfriend_avatar_pixbuf_cb, instance);
    connect_swapped(instance->objectfriend, "notify::connection-status", objectfriend_connection_status_cb, instance);
    connect_swapped(instance->objectfriend, "notify::status", objectfriend_status_cb, instance);
    connect(instance, "button-press-event", button_press_event_cb, NULL);
constructed_footer()

class_init()
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_friend_constructed;
    object_class->set_property = lupus_friend_set_property;

    define_property(PROP_OBJECTFRIEND, pointer, "objectfriend", G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

init() {}

LupusFriend *lupus_friend_new(LupusObjectFriend *objectfriend)
{
    return g_object_new(LUPUS_TYPE_FRIEND, "objectfriend", objectfriend, NULL);
}
