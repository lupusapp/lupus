#include "../include/lupus_mainfriend.h"
#include "../include/lupus_wrapper.h"
#include "../include/utils.h"
#include <sodium.h>

struct _LupusMainFriend {
    GtkEventBox parent_instance;

    guint32 friend_number;

    GtkMenu *menu;

    GtkButton *profile;
    GtkImage *profile_image;
    GtkLabel *name, *status_message;
};

G_DEFINE_TYPE(LupusMainFriend, lupus_mainfriend, GTK_TYPE_EVENT_BOX)

#define REMOVEFRIEND_DIALOG_WIDTH 250
#define REMOVEFRIEND_DIALOG_HEIGHT 100
#define REMOVEFRIEND_DIALOG_MARGIN 20
#define AVATAR_SIZE 48

#define WRAPPER_FRIEND (lupus_wrapper_get_friend(lupus_wrapper, instance->friend_number))

typedef enum {
    PROP_FRIEND_NUMBER = 1,
    N_PROPERTIES,
} LupusMainFriendProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

getter(mainfriend, MainFriend, friend_number, guint32);

static void lupus_mainfriend_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    LupusMainFriend *instance = LUPUS_MAINFRIEND(object);

    if ((LupusMainFriendProperty)property_id == PROP_FRIEND_NUMBER) {
        instance->friend_number = g_value_get_uint(value);
        return;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void lupus_mainfriend_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    LupusMainFriend *instance = LUPUS_MAINFRIEND(object);

    if ((LupusMainFriendProperty)property_id == PROP_FRIEND_NUMBER) {
        g_value_set_uint(value, instance->friend_number);
        return;
    }

    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void friend_notify_connection_cb(LupusMainFriend *instance)
{
    if (lupus_wrapperfriend_get_connection(WRAPPER_FRIEND) == TOX_CONNECTION_NONE) {
        remove_class_with_prefix(instance->profile, "profile--");
        gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(instance->profile)), "profile--offline");
    }
}

static void friend_notify_status_cb(LupusMainFriend *instance)
{
    remove_class_with_prefix(instance->profile, "profile--");

    gchar static const *classes[] = {
        "profile--none",
        "profile--away",
        "profile--busy",
    };
    gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
                                classes[lupus_wrapperfriend_get_status(WRAPPER_FRIEND)]);
}

static void friend_notify_status_message_cb(LupusMainFriend *instance)
{
    gchar *status_message = lupus_wrapperfriend_get_status_message(WRAPPER_FRIEND);
    gtk_label_set_text(instance->status_message, status_message);
    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->status_message), status_message);
}

static void friend_notify_name_cb(LupusMainFriend *instance)
{
    gchar *name = lupus_wrapperfriend_get_name(WRAPPER_FRIEND);
    gtk_label_set_text(instance->name, name);
    gtk_widget_set_tooltip_text(GTK_WIDGET(instance->name), name);
}

static void friend_notify_avatar_hash_cb(LupusMainFriend *instance)
{
    gchar *public_key = lupus_wrapperfriend_get_public_key(WRAPPER_FRIEND);
    gchar *filename = g_strconcat(LUPUS_TOX_DIR, "avatars/", public_key, ".png", NULL);

    gtk_image_set_from_pixbuf(instance->profile_image,
                              gdk_pixbuf_new_from_file_at_size(filename, AVATAR_SIZE, AVATAR_SIZE, NULL));

    g_free(filename);
}

static gboolean friend_button_press_event_cb(LupusMainFriend *instance, GdkEvent *event)
{
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button.button == 3) {
            gtk_menu_popup_at_pointer(instance->menu, event);
            return TRUE;
        }

        if (event->button.button == 1) {
            lupus_wrapper_set_active_chat_friend(lupus_wrapper, lupus_mainfriend_get_friend_number(instance));
        }
    }

    return FALSE;
}

static void menu_item_removefriend_activate_cb(LupusMainFriend *instance)
{
    GtkDialog *dialog = GTK_DIALOG(g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL));
    gtk_dialog_add_buttons(dialog, "Yes", 1, "No", 2, NULL);

    GtkBox *box = GTK_BOX(gtk_container_get_children(GTK_CONTAINER(dialog))->data);
    gtk_widget_set_margin_top(GTK_WIDGET(box), REMOVEFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_end(GTK_WIDGET(box), REMOVEFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_bottom(GTK_WIDGET(box), REMOVEFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_start(GTK_WIDGET(box), REMOVEFRIEND_DIALOG_MARGIN);

    gtk_box_pack_start(box, gtk_label_new("Remove friend ?"), TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(dialog));

    gtk_window_set_geometry_hints(GTK_WINDOW(dialog), NULL,
                                  &(GdkGeometry){.min_width = REMOVEFRIEND_DIALOG_WIDTH,
                                                 .min_height = REMOVEFRIEND_DIALOG_HEIGHT,
                                                 .max_width = REMOVEFRIEND_DIALOG_WIDTH,
                                                 .max_height = REMOVEFRIEND_DIALOG_HEIGHT},
                                  GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE); // NOLINT

    if (gtk_dialog_run(dialog) == 1) {
        lupus_wrapper_remove_friend(lupus_wrapper, instance->friend_number);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void init_popover(LupusMainFriend *instance)
{
    GtkBox *menu_item_removefriend_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_pack_start(menu_item_removefriend_box, gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_MENU),
                       FALSE, TRUE, 0);
    gtk_box_pack_start(menu_item_removefriend_box, gtk_label_new("Remove friend"), TRUE, TRUE, 0);

    GtkWidget *menu_item_removefriend = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(menu_item_removefriend), GTK_WIDGET(menu_item_removefriend_box));

    instance->menu = GTK_MENU(gtk_menu_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(instance->menu), menu_item_removefriend);

    gtk_widget_show_all(GTK_WIDGET(instance->menu));

    g_signal_connect_swapped(menu_item_removefriend, "activate", G_CALLBACK(menu_item_removefriend_activate_cb),
                             instance);
}

static void lupus_mainfriend_constructed(GObject *object)
{
    LupusMainFriend *instance = LUPUS_MAINFRIEND(object);
    LupusWrapperFriend *friend = WRAPPER_FRIEND;

    friend_notify_name_cb(instance);
    friend_notify_status_message_cb(instance);
    friend_notify_avatar_hash_cb(instance);

    init_popover(instance);

    g_signal_connect_swapped(friend, "notify::name", G_CALLBACK(friend_notify_name_cb), instance);
    g_signal_connect_swapped(friend, "notify::status-message", G_CALLBACK(friend_notify_status_message_cb), instance);
    g_signal_connect_swapped(friend, "notify::status", G_CALLBACK(friend_notify_status_cb), instance);
    g_signal_connect_swapped(friend, "notify::connection", G_CALLBACK(friend_notify_connection_cb), instance);
    g_signal_connect_swapped(friend, "notify::avatar-hash", G_CALLBACK(friend_notify_avatar_hash_cb), instance);

    g_signal_connect(instance, "button-press-event", G_CALLBACK(friend_button_press_event_cb), NULL);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_mainfriend_parent_class); // NOLINT
    object_class->constructed(object);
}

static void lupus_mainfriend_class_init(LupusMainFriendClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass *object_class = G_OBJECT_CLASS(class); // NOLINT

    gtk_widget_class_set_template_from_resource(widget_class, LUPUS_RESOURCES "/mainfriend.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusMainFriend, profile);
    gtk_widget_class_bind_template_child(widget_class, LupusMainFriend, profile_image);
    gtk_widget_class_bind_template_child(widget_class, LupusMainFriend, name);
    gtk_widget_class_bind_template_child(widget_class, LupusMainFriend, status_message);

    object_class->constructed = lupus_mainfriend_constructed;
    object_class->set_property = lupus_mainfriend_set_property;
    object_class->get_property = lupus_mainfriend_get_property;

    gint param = G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY; // NOLINT
    obj_properties[PROP_FRIEND_NUMBER] =
        g_param_spec_uint("friend-number", "Friend number", "Friend number", 0, UINT32_MAX, 0, param);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_mainfriend_init(LupusMainFriend *instance) { gtk_widget_init_template(GTK_WIDGET(instance)); }

LupusMainFriend *lupus_mainfriend_new(guint32 friend_number)
{
    return g_object_new(LUPUS_TYPE_MAINFRIEND, "friend-number", friend_number, NULL);
}