#include "../include/lupus_main.h"
#include "../include/lupus.h"
#include "../include/lupus_editablelabel.h"
#include "../include/lupus_friend.h"
#include "../include/lupus_profile.h"
#include "glibconfig.h"
#include "include/lupus_objectfriend.h"
#include "pango/pango-layout.h"
#include <tox/tox.h>

struct _LupusMain {
    GtkApplicationWindow parent_instance;

    LupusObjectSelf *objectself;
    LupusProfile *profile;

    GtkBox *box;
    GtkBox *sidebox;
    GtkScrolledWindow *sidebox_friends_scrolled_window;
    GtkBox *sidebox_friends_box;
    GtkActionBar *sidebox_actionbar;
};

G_DEFINE_TYPE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

#define t_n lupus_main
#define TN  LupusMain
#define T_N LUPUS_MAIN

declare_properties(PROP_OBJECTSELF);

static void remove_lupusfriend(INSTANCE, LupusObjectFriend *objectfriend)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(instance->sidebox_friends_box));

    for (GList *child = children; child; child = child->next) {
        gpointer data = child->data;

        if (!LUPUS_IS_FRIEND(data)) {
            continue;
        }

        if (data != objectfriend) {
            continue;
        }

        gtk_widget_destroy(GTK_WIDGET(data));

        GList *next = child->next;
        if (next && GTK_IS_SEPARATOR(next->data)) {
            gtk_widget_destroy(GTK_WIDGET(next->data));
        }

        break;
    }

    g_list_free(children);
}

static void add_lupusfriend(INSTANCE, LupusObjectFriend *objectfriend)
{
    GtkWidget *friend = GTK_WIDGET(lupus_friend_new(objectfriend));
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    gtk_box_pack_start(instance->sidebox_friends_box, friend, FALSE, TRUE, 0);
    gtk_box_pack_start(instance->sidebox_friends_box, separator, FALSE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(instance->sidebox_friends_box));
}

static void construct_sidebox_friends(INSTANCE)
{
    instance->sidebox_friends_scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    instance->sidebox_friends_box = GTK_BOX(
        g_object_new(GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 5, "spacing", 5, NULL));

    gtk_container_add(GTK_CONTAINER(instance->sidebox_friends_scrolled_window),
                      GTK_WIDGET(instance->sidebox_friends_box));
    gtk_box_pack_start(instance->sidebox, GTK_WIDGET(instance->sidebox_friends_scrolled_window), TRUE, TRUE, 0);

    object_get_prop(instance->objectself, "objectfriends", objectfriends, GHashTable *);
    if (!objectfriends) {
        return;
    }

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, objectfriends);
    gpointer value = NULL;

    while (g_hash_table_iter_next(&iter, NULL, &value)) {
        add_lupusfriend(instance, LUPUS_OBJECTFRIEND(value));
    }
}

static void sidebox_actionbar_about_activate_cb(void)
{
    static GtkAboutDialog *about_dialog;

    if (!about_dialog) {
        GdkPixbuf *pixbuf = GDK_PIXBUF(gdk_pixbuf_new_from_resource(LUPUS_RESOURCES "/lupus.svg", NULL));

        about_dialog = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
        gtk_about_dialog_set_program_name(about_dialog, "Lupus");
        gtk_about_dialog_set_version(about_dialog, LUPUS_VERSION);
        gtk_about_dialog_set_copyright(about_dialog, "© 2019-2020 Ogromny");
        gtk_about_dialog_set_wrap_license(about_dialog, TRUE);
        gtk_about_dialog_set_license_type(about_dialog, GTK_LICENSE_GPL_3_0);
        gtk_about_dialog_set_website(about_dialog, "https://github.com/LupusApp/Lupus");
        gtk_about_dialog_set_website_label(about_dialog, "GitHub");
        gtk_about_dialog_set_authors(about_dialog, (gchar const *[]){"Ogromny", NULL});
        gtk_about_dialog_set_logo(about_dialog, pixbuf);
    }

    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_hide(GTK_WIDGET(about_dialog));
}

static void sidebox_actionbar_button_add_friend_clicked_cb(INSTANCE)
{
    static GtkDialog *dialog;
    static GtkEntry *address_hex;
    static GtkEntry *message;
    static gchar const *default_message = "Hi, please add me to your friends :D!";

    if (!dialog) {
        dialog = GTK_DIALOG(g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, "title", "Add friend", "border-width",
                                         5, "resizable", FALSE, NULL));
        gtk_dialog_add_buttons(dialog, "Add", GTK_RESPONSE_YES, "Cancel", GTK_RESPONSE_CANCEL, NULL);

        address_hex = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(address_hex, tox_max_message_length());
        gtk_entry_set_placeholder_text(address_hex, "Friend public key");

        message = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(message, tox_max_friend_request_length());
        gtk_entry_set_placeholder_text(message, "Request message");
        gtk_entry_set_text(message, default_message);

        GtkBox *box = GTK_BOX(gtk_container_get_children(GTK_CONTAINER(dialog))->data);
        gtk_box_set_spacing(box, 5);
        gtk_box_pack_start(box, GTK_WIDGET(address_hex), TRUE, TRUE, 0);
        gtk_box_pack_start(box, GTK_WIDGET(message), TRUE, TRUE, 0);

        gtk_widget_show_all(GTK_WIDGET(box));
    }

    if (gtk_dialog_run(dialog) == GTK_RESPONSE_YES) {
        gsize address_hex_length = gtk_entry_get_text_length(address_hex);

        if (!address_hex_length) {
            lupus_error("Public key required.");
            goto end;
        }

        if (address_hex_length != tox_address_size() * 2) {
            lupus_error("This public key is invalid.");
            goto end;
        }

        gboolean success = FALSE;
        gchar const *request_address_hex = gtk_entry_get_text(address_hex);
        gchar const *request_message = gtk_entry_get_text(message);

        emit_by_name(instance->objectself, "add-friend", request_address_hex, request_message, &success);
    }

end:
    gtk_widget_hide(GTK_WIDGET(dialog));
    gtk_entry_set_text(address_hex, "");
    gtk_entry_set_text(message, default_message);
}

static void construct_sidebox_actionbar(INSTANCE)
{
    instance->sidebox_actionbar = GTK_ACTION_BAR(gtk_action_bar_new());

    GtkBox *popover_about_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    GtkWidget *popover_about_image = gtk_image_new_from_icon_name("help-about", GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(popover_about_box, popover_about_image, FALSE, TRUE, 0);
    gtk_box_pack_start(popover_about_box, gtk_label_new("About"), TRUE, TRUE, 0);
    GtkWidget *popover_about = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(popover_about), GTK_WIDGET(popover_about_box));

    GtkWidget *popover = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(popover), popover_about);
    gtk_widget_show_all(popover);

    GtkMenuButton *button_settings = GTK_MENU_BUTTON(gtk_menu_button_new());
    gtk_menu_button_set_popup(button_settings, GTK_WIDGET(popover));
    GtkContainer *button_settings_container = GTK_CONTAINER(button_settings);
    GtkWidget *button_settings_image = gtk_image_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_container_add(button_settings_container, button_settings_image);

    GtkButton *button_add_friend = GTK_BUTTON(gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_SMALL_TOOLBAR));
    gtk_widget_set_tooltip_text(GTK_WIDGET(button_add_friend), "Add a friend");

    gtk_action_bar_pack_start(instance->sidebox_actionbar, GTK_WIDGET(button_add_friend));
    gtk_action_bar_pack_start(instance->sidebox_actionbar, GTK_WIDGET(button_settings));

    gtk_box_pack_end(instance->sidebox_friends_box, GTK_WIDGET(instance->sidebox_actionbar), FALSE, TRUE, 0);

    connect_swapped(popover_about, "activate", sidebox_actionbar_about_activate_cb, instance);
    connect_swapped(button_add_friend, "clicked", sidebox_actionbar_button_add_friend_clicked_cb, instance);
}

typedef struct {
    gboolean accept;
    GMainLoop *main_loop;
} FriendRequestRunInfo;

static void friend_request_infobar_response_cb(GtkInfoBar *infobar, gint response_id, FriendRequestRunInfo *runinfo)
{
    runinfo->accept = (response_id == GTK_RESPONSE_ACCEPT);

    if (g_main_loop_is_running(runinfo->main_loop)) {
        g_main_loop_quit(runinfo->main_loop);
    }

    gtk_info_bar_set_revealed(infobar, FALSE);
}

static gboolean friend_request_infobar_destroy(GtkWidget *infobar_widget)
{
    gtk_widget_destroy(infobar_widget);
    return FALSE;
}

static gboolean friend_request_cb(INSTANCE, gchar *public_key_hex, gchar *request_message)
{
    gchar *label_preformat_text = "<sup><b>Friend request</b> from <i>%s</i></sup>\n%s";
    gchar label_text[strlen(label_preformat_text) + strlen(public_key_hex) + strlen(request_message) + 1];
    g_snprintf(label_text, sizeof(label_text), label_preformat_text, public_key_hex, request_message);

    GtkLabel *label = GTK_LABEL(gtk_label_new(label_text));
    gtk_label_set_use_markup(label, TRUE);
    gtk_label_set_selectable(label, TRUE);
    gtk_label_set_line_wrap_mode(label, PANGO_WRAP_WORD_CHAR);
    gtk_label_set_line_wrap(label, TRUE);

    GtkInfoBar *infobar =
        GTK_INFO_BAR(gtk_info_bar_new_with_buttons("Accept", GTK_RESPONSE_ACCEPT, "Reject", GTK_RESPONSE_REJECT, NULL));
    gtk_info_bar_set_message_type(infobar, GTK_MESSAGE_QUESTION);

    GtkContainer *infobar_content = GTK_CONTAINER(gtk_info_bar_get_content_area(infobar));
    gtk_container_add(infobar_content, GTK_WIDGET(label));

    GtkWidget *infobar_widget = GTK_WIDGET(infobar);
    gtk_box_pack_start(instance->box, infobar_widget, FALSE, TRUE, 0);
    gtk_box_reorder_child(instance->box, infobar_widget, 0);

    gtk_info_bar_set_revealed(infobar, FALSE);
    gtk_widget_show_all(infobar_widget);
    gtk_info_bar_set_revealed(infobar, TRUE);

    FriendRequestRunInfo runinfo = {FALSE, g_main_loop_new(NULL, FALSE)};
    gulong response_handler = connect(infobar, "response", friend_request_infobar_response_cb, &runinfo);

    g_main_loop_run(runinfo.main_loop);
    g_main_loop_unref(runinfo.main_loop);

    disconnect(infobar, response_handler);

    g_timeout_add(2000, G_SOURCE_FUNC(friend_request_infobar_destroy), infobar);

    return runinfo.accept;
}

get_property_header()
case PROP_OBJECTSELF:
    g_value_set_pointer(value, instance->objectself);
    break;
get_property_footer()

set_property_header()
case PROP_OBJECTSELF:
    instance->objectself = g_value_get_pointer(value);
    break;
set_property_footer()

constructed_header()
    instance->profile = lupus_profile_new(instance->objectself);
    gtk_box_pack_start(instance->sidebox, GTK_WIDGET(instance->profile), FALSE, TRUE, 0);
    gtk_box_pack_start(instance->sidebox, gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, TRUE, 0);

    construct_sidebox_friends(instance);
    construct_sidebox_actionbar(instance);

    gtk_widget_show_all(GTK_WIDGET(instance));

    connect_swapped(instance->objectself, "friend-request", friend_request_cb, instance);
    connect_swapped(instance->objectself, "friend-added", add_lupusfriend, instance);
    connect_swapped(instance->objectself, "friend-removed", remove_lupusfriend, instance);
constructed_footer()

init() { gtk_widget_init_template(GTK_WIDGET(instance)); }

class_init()
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    gtk_widget_class_set_template_from_resource(widget_class, LUPUS_RESOURCES "/main.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusMain, box);
    gtk_widget_class_bind_template_child(widget_class, LupusMain, sidebox);

    GObjectClass *object_class = G_OBJECT_CLASS(class);
    object_class->constructed = lupus_main_constructed;
    object_class->set_property = lupus_main_set_property;
    object_class->get_property = lupus_main_get_property;

    define_property(PROP_OBJECTSELF, pointer, "objectself", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

LupusMain *lupus_main_new(GtkApplication *application, LupusObjectSelf *objectself)
{
    return g_object_new(LUPUS_TYPE_MAIN, "application", application, "objectself", objectself, NULL);
}

