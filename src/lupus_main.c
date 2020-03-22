#include "../include/lupus_main.h"
#include "../include/lupus.h"
#include "../include/lupus_mainchat.h"
#include "../include/lupus_mainfriend.h"
#include "../include/lupus_mainheaderbar.h"
#include "../include/lupus_wrapper.h"
#include <sodium.h>

struct _LupusMain {
    GtkApplicationWindow parent_instance;

    LupusMainHeaderBar *main_header_bar;
    GtkBox *box, *parent_box;

    LupusMainChat *active_chat;
};

static GHashTable *mainchats;
static GHashTable *mainfriends;
static LupusMain *static_instance;

G_DEFINE_TYPE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

#define ADDFRIEND_DIALOG_WIDTH 350
#define ADDFRIEND_DIALOG_HEIGHT 200
#define ADDFRIEND_DIALOG_MARGIN 20
#define ADDFRIEND_DIALOG_BOX_SPACING 5
#define SEPARATOR_MARGIN 5
#define FRIENDLIST_WIDTH 350

/*
 * 1.0.0
 * ---
 * TODO(ogromny): chat basic
 * TODO(ogromny): toupper all address
 *
 * 1.0.1
 * ---
 * TODO(ogromny): Ensure windows and macos compatibility
 *
 * 1.1.0
 * ---
 * TODO(ogromny): add notification center
 *
 * 1.2.0
 * ---
 * TODO(ogromny): add settings
 */

static gboolean friend_list_button_press_event_cb(GtkMenu *menu,
                                                  GdkEvent *event) {
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        gtk_menu_popup_at_pointer(menu, event);
    }
    return FALSE;
}

static void menu_item_addfriend_activate_cb() {
    GtkDialog *dialog = GTK_DIALOG(g_object_new(
        GTK_TYPE_DIALOG, "use-header-bar", TRUE, "title", "Add friend", NULL));
    gtk_dialog_add_button(dialog, "Add", 1);

    GtkBox *box =
        GTK_BOX(gtk_container_get_children(GTK_CONTAINER(dialog))->data);
    gtk_widget_set_margin_top(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_end(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_bottom(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_start(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);
    gtk_box_set_spacing(box, ADDFRIEND_DIALOG_BOX_SPACING);

    GtkEntry *friend_hex = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_max_length(friend_hex, TOX_ADDRESS_SIZE * 2);
    gtk_entry_set_placeholder_text(friend_hex, "Friend address");
    gtk_widget_set_valign(GTK_WIDGET(friend_hex), GTK_ALIGN_CENTER);

    GtkEntry *friend_message = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_max_length(friend_message, TOX_MAX_FRIEND_REQUEST_LENGTH);
    gtk_entry_set_placeholder_text(friend_message, "Friend message request");
    gtk_widget_set_valign(GTK_WIDGET(friend_message), GTK_ALIGN_CENTER);

    gtk_box_pack_start(box, GTK_WIDGET(friend_hex), TRUE, TRUE, 0);
    gtk_box_pack_start(box, GTK_WIDGET(friend_message), TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(dialog));

    gtk_window_set_geometry_hints(
        GTK_WINDOW(dialog), NULL,
        &(GdkGeometry){.min_width = ADDFRIEND_DIALOG_WIDTH,
                       .min_height = ADDFRIEND_DIALOG_HEIGHT,
                       .max_width = ADDFRIEND_DIALOG_WIDTH,
                       .max_height = ADDFRIEND_DIALOG_HEIGHT},
        GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE); // NOLINT

    if (gtk_dialog_run(dialog) == 1) {
        gchar const *address_hex = gtk_entry_get_text(friend_hex);
        if (!(*address_hex)) {
            lupus_error("Please enter an address.");
            goto end;
        }

        guchar address_bin[TOX_ADDRESS_SIZE];
        gchar const *message = gtk_entry_get_text(friend_message);

        sodium_hex2bin(address_bin, sizeof(address_bin), address_hex,
                       strlen(address_hex), NULL, NULL, NULL);

        lupus_wrapper_add_friend(lupus_wrapper, address_bin, (guint8 *)message,
                                 strlen(message));
    }

end:
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void wrapper_notify_friends_cb(GtkBox *box) {
    /* Temporary GHashTable (will contains all existing friends) */
    GHashTable *tmp = g_hash_table_new(NULL, NULL);

    GList *friends =
        g_hash_table_get_values(lupus_wrapper_get_friends(lupus_wrapper));
    for (GList *i = friends; i; i = i->next) {
        guint friend_number =
            lupus_wrapperfriend_get_id(LUPUS_WRAPPERFRIEND(i->data));
        gpointer key = GUINT_TO_POINTER(friend_number);

        LupusMainFriend *friend = g_hash_table_lookup(mainfriends, key);

        /* If friend already exist push it in tmp */
        if (friend) {
            g_hash_table_insert(tmp, key, friend);
            continue;
        }

        /* Otherwise create it and push it in mainfriends and tmp */
        friend = lupus_mainfriend_new(friend_number);
        gtk_box_pack_start(box, GTK_WIDGET(friend), FALSE, TRUE, 0);

        GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_set_margin_start(separator, SEPARATOR_MARGIN);
        gtk_widget_set_margin_end(separator, SEPARATOR_MARGIN);
        gtk_box_pack_start(box, separator, FALSE, TRUE, 0);

        g_hash_table_insert(mainfriends, key, friend);
        g_hash_table_insert(tmp, key, friend);
    }
    g_list_free(friends);

    /* Check if all friend in mainfriends is present in tmp
     * If not delete it from mainfriends and mainchats
     */
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, mainfriends);
    for (gpointer key = NULL, value = NULL;
         g_hash_table_iter_next(&iter, &key, &value);) {
        if (g_hash_table_contains(tmp, key)) {
            continue;
        }

        /* Delete from mainfriends */
        GList *children = gtk_container_get_children(GTK_CONTAINER(box));
        for (GList *child = children; child; child = child->next) {
            /* Delete mainfriend and the next separator */
            if (child->data == value) {
                gtk_widget_destroy(GTK_WIDGET(child->data));
                gtk_widget_destroy(GTK_WIDGET(child->next->data));
                break;
            }
        }
        g_list_free(children);
        g_hash_table_iter_remove(&iter);

        /* Delete from mainchats */
        LupusMainChat *main_chat =
            LUPUS_MAINCHAT(g_hash_table_lookup(mainchats, key));
        if (main_chat) {
            if (main_chat == static_instance->active_chat) {
                lupus_mainheaderbar_reset_titles(
                    static_instance->main_header_bar);
                static_instance->active_chat = NULL;
            }

            gtk_widget_destroy(GTK_WIDGET(main_chat));
            g_hash_table_remove(mainchats, key);
        }
    }

    g_hash_table_destroy(tmp);
    gtk_widget_show_all(GTK_WIDGET(box));
}

static void init_friend_list(LupusMain *instance) {
    GtkBox *menu_item_addfriend_box =
        GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_pack_start(
        menu_item_addfriend_box,
        gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_MENU), FALSE,
        TRUE, 0);
    gtk_box_pack_start(menu_item_addfriend_box, gtk_label_new("Add friend"),
                       TRUE, TRUE, 0);

    GtkWidget *menu_item_addfriend = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(menu_item_addfriend),
                      GTK_WIDGET(menu_item_addfriend_box));

    GtkWidget *menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item_addfriend);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(box, FRIENDLIST_WIDTH, 0);

    GtkWidget *friend_list = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(friend_list), box);

    gtk_box_pack_start(instance->box, friend_list, FALSE, TRUE, 0);

    gtk_widget_show_all(menu);

    wrapper_notify_friends_cb(GTK_BOX(box));

    g_signal_connect(menu_item_addfriend, "activate",
                     G_CALLBACK(menu_item_addfriend_activate_cb), NULL);

    g_signal_connect_swapped(friend_list, "button-press-event",
                             G_CALLBACK(friend_list_button_press_event_cb),
                             menu);

    g_signal_connect_swapped(lupus_wrapper, "notify::friends",
                             G_CALLBACK(wrapper_notify_friends_cb), box);
}

static void wrapper_notify_active_chat_friend_cb(LupusMain *instance) {
    gpointer key =
        GUINT_TO_POINTER(lupus_wrapper_get_active_chat_friend(lupus_wrapper));

    if (instance->active_chat) {
        g_object_ref(instance->active_chat);
        gtk_container_remove(GTK_CONTAINER(instance->box),
                             GTK_WIDGET(instance->active_chat));
    }

    LupusMainChat *chat = g_hash_table_lookup(mainchats, key);
    if (!chat) {
        chat = lupus_mainchat_new();
        g_hash_table_insert(mainchats, key, chat);
    }

    gtk_box_pack_start(instance->box, GTK_WIDGET(chat), TRUE, TRUE, 0);
    instance->active_chat = chat;
}

static gboolean destroy_return_false(gpointer widget) {
    gtk_widget_destroy(widget);
    return FALSE;
}

static void friend_request_response_cb(GtkInfoBar *info_bar, gint response_id,
                                       guint8 *address) {
    if (response_id == 1) {
        lupus_wrapper_add_friend(lupus_wrapper, address, NULL, 0);
    }
    g_free(address);

    gtk_info_bar_set_revealed(info_bar, FALSE);
    /* Wait 2s to be sure that the widget disappear */
    g_timeout_add_seconds(2, destroy_return_false, info_bar);
}

static void friend_request_cb(Tox *tox, // NOLINT
                              guint8 const *public_key, guint8 const *message,
                              gsize length, gpointer user_data) { // NOLINT
    gchar address_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    sodium_bin2hex(address_hex, sizeof(address_hex), public_key,
                   TOX_PUBLIC_KEY_SIZE);

    gchar *address = g_ascii_strup(address_hex, sizeof(address_hex));
    gchar *text = g_strdup_printf(
        "<sup><b>Friend request</b></sup>\n<sub><i>%s</i></sub>\n%s", address,
        message);

    GtkLabel *label = GTK_LABEL(gtk_label_new(text));
    gtk_label_set_use_markup(label, TRUE);
    gtk_label_set_selectable(label, TRUE);
    gtk_label_set_line_wrap_mode(label, PANGO_WRAP_WORD_CHAR);
    gtk_label_set_line_wrap(label, TRUE);

    g_free(address);
    g_free(text);

    GtkInfoBar *info_bar = GTK_INFO_BAR(
        gtk_info_bar_new_with_buttons("Accept", 1, "Reject", 2, NULL));
    gtk_info_bar_set_message_type(info_bar, GTK_MESSAGE_QUESTION);
    gtk_container_add(GTK_CONTAINER(gtk_info_bar_get_content_area(info_bar)),
                      GTK_WIDGET(label));

    gtk_box_pack_start(static_instance->parent_box, GTK_WIDGET(info_bar), FALSE,
                       TRUE, 0);
    gtk_box_reorder_child(static_instance->parent_box, GTK_WIDGET(info_bar), 0);

    gtk_widget_show_all(GTK_WIDGET(info_bar));
    gtk_info_bar_set_revealed(info_bar, TRUE);

    g_signal_connect(info_bar, "response",
                     G_CALLBACK(friend_request_response_cb),
                     g_memdup(public_key, TOX_PUBLIC_KEY_SIZE));
}

static void lupus_main_finalize(GObject *object) {
    /* FIXME: clean by hand */

    g_hash_table_destroy(mainchats);
    g_hash_table_destroy(mainfriends);

    G_OBJECT_CLASS(lupus_main_parent_class)->finalize(object); // NOLINT
}

static void lupus_main_class_init(LupusMainClass *class) {
    mainchats = g_hash_table_new(NULL, NULL);
    mainfriends = g_hash_table_new(NULL, NULL);

    G_OBJECT_CLASS(class)->finalize = lupus_main_finalize; // NOLINT
}

static void lupus_main_init(LupusMain *instance) {
    static_instance = instance;

    instance->active_chat = NULL;

    instance->main_header_bar = lupus_mainheaderbar_new();
    gtk_window_set_titlebar(GTK_WINDOW(instance),
                            GTK_WIDGET(instance->main_header_bar));

    /*
     * box
     */
    instance->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

    init_friend_list(instance);

    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_margin_top(separator, SEPARATOR_MARGIN);
    gtk_widget_set_margin_bottom(separator, SEPARATOR_MARGIN);
    gtk_box_pack_start(instance->box, separator, FALSE, TRUE, 0);

    /*
     * parent_box
     */
    instance->parent_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_box_pack_start(instance->parent_box, GTK_WIDGET(instance->box), TRUE,
                       TRUE, 0);

    gtk_container_add(GTK_CONTAINER(instance),
                      GTK_WIDGET(instance->parent_box));

    gtk_widget_show_all(GTK_WIDGET(instance->parent_box));

    lupus_wrapper_bootstrap(lupus_wrapper);
    lupus_wrapper_start_listening(lupus_wrapper);

    tox_callback_friend_request(lupus_wrapper_get_tox(lupus_wrapper),
                                friend_request_cb);

    g_signal_connect_swapped(lupus_wrapper, "notify::active-chat-friend",
                             G_CALLBACK(wrapper_notify_active_chat_friend_cb),
                             instance);
}

LupusMain *lupus_main_new(GtkApplication *application) {
    return g_object_new(LUPUS_TYPE_MAIN, "application", application, NULL);
}