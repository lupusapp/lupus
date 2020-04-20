#include "../include/lupus_mainchat.h"
#include "../include/lupus_mainmessage.h"
#include "../include/lupus_wrapper.h"

struct _LupusMainChat {
    GtkEventBox parent_instance;

    GPtrArray *messages;

    GtkEntry *entry;
    GtkButton *submit;
    GtkBox *chat_box;
};

G_DEFINE_TYPE(LupusMainChat, lupus_mainchat, GTK_TYPE_EVENT_BOX)

typedef enum {
    SUBMIT,
    LAST_SIGNAL,
} LupusMainChatSignal;
static guint signals[LAST_SIGNAL];

void lupus_mainchat_add_message(LupusMainChat *instance, LupusWrapperFriend *friend, gchar const *text)
{
    /*
     * No message, so create it
     */
    if (!instance->messages->len) {
        goto end;
    }

    /*
     * Append if text author is the author of the last message
     */
    LupusMainMessage *last_message = g_ptr_array_index(instance->messages, instance->messages->len - 1);
    if (friend != NULL == !lupus_mainmessage_is_myself(last_message)) {
        lupus_mainmessage_append(last_message, text);
        return;
    }

end:;
    GtkWidget *message = GTK_WIDGET(lupus_mainmessage_new(friend, text));
    g_ptr_array_add(instance->messages, message);
    gtk_box_pack_start(instance->chat_box, GTK_WIDGET(message), FALSE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(instance->chat_box));
}

static void submit_cb(LupusMainChat *instance)
{
    gchar const *text = gtk_entry_get_text(instance->entry);

    g_signal_emit(instance, signals[SUBMIT], 0, text);
    lupus_mainchat_add_message(instance, NULL, text);

    gtk_entry_set_text(instance->entry, "");
}

static gboolean entry_key_press_event_cb(LupusMainChat *instance, GdkEvent *event)
{
    if (event->type == GDK_KEY_PRESS && event->key.keyval == GDK_KEY_Return) {
        if (g_strcmp0(gtk_entry_get_text(instance->entry), "")) {
            submit_cb(instance);
        }
    }
    return FALSE;
}

static void lupus_mainchat_finalize(GObject *object)
{
    LupusMainChat *instance = LUPUS_MAINCHAT(object);

    /*
     * No need to free each element by hand, because it's already the case, `chat_box` is already dropped
     */
    g_ptr_array_free(instance->messages, TRUE);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_mainchat_parent_class); // NOLINT
    object_class->finalize(object);
}

static void lupus_mainchat_class_init(LupusMainChatClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    gtk_widget_class_set_template_from_resource(widget_class, LUPUS_RESOURCES "/mainchat.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusMainChat, entry);
    gtk_widget_class_bind_template_child(widget_class, LupusMainChat, submit);
    gtk_widget_class_bind_template_child(widget_class, LupusMainChat, chat_box);

    GObjectClass *object_class = G_OBJECT_CLASS(class); // NOLINT
    object_class->finalize = lupus_mainchat_finalize;

    signals[SUBMIT] =
        g_signal_new("submit", LUPUS_TYPE_MAINCHAT, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, // NOLINT
                     1, G_TYPE_STRING);                                                                  // NOLINT
}

static void lupus_mainchat_init(LupusMainChat *instance)
{
    gtk_widget_init_template(GTK_WIDGET(instance));

    instance->messages = g_ptr_array_new();

    g_signal_connect_swapped(instance->submit, "clicked", G_CALLBACK(submit_cb), instance);
    g_signal_connect_swapped(instance->entry, "key-press-event", G_CALLBACK(entry_key_press_event_cb), instance);
}

LupusMainChat *lupus_mainchat_new(void) { return g_object_new(LUPUS_TYPE_MAINCHAT, NULL); }