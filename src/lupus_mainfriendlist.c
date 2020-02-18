#include "../include/lupus_mainfriendlist.h"
#include "../include/lupus.h"

struct _LupusMainFriendList {
    GtkEventBox parent_instance;

    GtkBox *box;
    GtkMenu *list_menu;
    GtkMenuItem *list_menu_addfriend;
};

G_DEFINE_TYPE(LupusMainFriendList, lupus_mainfriendlist, GTK_TYPE_EVENT_BOX)

gboolean click_cb(LupusMainFriendList *instance, GdkEvent *event) {
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        gtk_menu_popup_at_pointer(instance->list_menu, event);
    }
    return FALSE;
}

static void lupus_mainfriendlist_class_init(LupusMainFriendListClass *class) {
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(class), LUPUS_RESOURCES "/mainfriendlist.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainFriendList, box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LupusMainFriendList, list_menu);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(class), LupusMainFriendList, list_menu_addfriend);
}

static void lupus_mainfriendlist_init(LupusMainFriendList *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect(instance, "button-press-event", G_CALLBACK(click_cb),
                     NULL);
}

LupusMainFriendList *lupus_mainfriendlist_new(void) {
    return g_object_new(LUPUS_TYPE_MAINFRIENDLIST, NULL);
}