#include "../include/lupus_mainchat.h"
#include "../include/lupus.h"
#include "../include/lupus_wrapper.h"
#include "../include/lupus_wrapperfriend.h"

struct _LupusMainChat {
    GtkEventBox parent_instance;

    GtkEntry *submit;
    GtkBox *chat_box;
};

G_DEFINE_TYPE(LupusMainChat, lupus_mainchat, GTK_TYPE_EVENT_BOX)

static void lupus_mainchat_class_init(LupusMainChatClass *class) {
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    gtk_widget_class_set_template_from_resource(widget_class,
                                                LUPUS_RESOURCES "/mainchat.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusMainChat, submit);
    gtk_widget_class_bind_template_child(widget_class, LupusMainChat, chat_box);
}

static void lupus_mainchat_init(LupusMainChat *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));
}

LupusMainChat *lupus_mainchat_new(void) {
    return g_object_new(LUPUS_TYPE_MAINCHAT, NULL);
}