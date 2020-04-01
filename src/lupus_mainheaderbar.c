#include "../include/lupus_mainheaderbar.h"
#include "../include/lupus.h"
#include "../include/lupus_editablelabel.h"
#include "../include/lupus_wrapper.h"
#include "../include/utils.h"
#include <glib/gstdio.h>
#include <sodium/utils.h>

struct _LupusMainHeaderBar {
    GtkBox parent_instance;

    guint active_chat_friend_notify_name_handler_id;
    guint active_chat_friend_notify_status_message_handler_id;

    GtkHeaderBar *left_headerbar, *right_headerbar;
    GtkButton *profile;
    GtkImage *profile_image;
    GtkMenu *profile_popover, *menu_popover;
    GtkMenuItem *profile_popover_none, *profile_popover_away,
        *profile_popover_busy, *profile_popover_toxid;
    GtkPopover *popover;
    GtkButton *profile_bigger;
    GtkImage *profile_bigger_image;
    GtkBox *vbox;
    LupusEditableLabel *name, *status_message;
};

G_DEFINE_TYPE(LupusMainHeaderBar, lupus_mainheaderbar, GTK_TYPE_BOX)

#define TOXID_DIALOG_MARGIN 20
#define FILE_CHOOSER_DIALOG_PREVIEW_SIZE 128
#define AVATAR_MAX_FILE_SIZE 65536
#define AVATAR_SIZE 64
#define AVATAR_SIZE_MINI 36

void lupus_mainheaderbar_reset_titles(LupusMainHeaderBar *instance) {
    gtk_header_bar_set_title(instance->right_headerbar, NULL);
    gtk_header_bar_set_subtitle(instance->right_headerbar, NULL);
}

static gboolean profile_button_press_event(LupusMainHeaderBar *instance,
                                           GdkEvent *event) {
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        gtk_menu_popup_at_pointer(instance->profile_popover, event);
    }
    return FALSE;
}

static gboolean name_submit_cb(gpointer _, gchar *value) { // NOLINT
    lupus_wrapper_set_name(lupus_wrapper, value);
    lupus_wrapper_save(lupus_wrapper);
    return TRUE;
}

static gboolean status_message_submit_cb(gpointer _, gchar *value) { // NOLINT
    lupus_wrapper_set_status_message(lupus_wrapper, value);
    lupus_wrapper_save(lupus_wrapper);
    return TRUE;
}

static void wrapper_notify_name_cb(LupusMainHeaderBar *instance) {
    lupus_editablelabel_set_value(instance->name,
                                  lupus_wrapper_get_name(lupus_wrapper));
}

static void wrapper_notify_status_message_cb(LupusMainHeaderBar *instance) {
    lupus_editablelabel_set_value(
        instance->status_message,
        lupus_wrapper_get_status_message(lupus_wrapper));
}

static void wrapper_notify_status_cb(LupusMainHeaderBar *instance) {
    gchar static const *class_name[] = {
        "profile--none",
        "profile--away",
        "profile--busy",
    };
    gint static const offset[] = {
        offsetof(LupusMainHeaderBar, profile_popover_none),
        offsetof(LupusMainHeaderBar, profile_popover_away),
        offsetof(LupusMainHeaderBar, profile_popover_busy),
    };

    Tox_User_Status status = lupus_wrapper_get_status(lupus_wrapper);
    gpointer actual = *(gpointer *)((gchar *)instance + offset[status]);
    for (gsize i = 0, j = G_N_ELEMENTS(offset); i < j; ++i) {
        GtkWidget *w = GTK_WIDGET(*(gpointer *)((gchar *)instance + offset[i]));
        gtk_widget_set_sensitive(w, (w == actual) ? FALSE : TRUE);
    }

    remove_class_with_prefix(instance->profile, "profile--");
    remove_class_with_prefix(instance->profile_bigger, "profile--");

    gtk_style_context_add_class(
        gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
        class_name[status]);
    gtk_style_context_add_class(
        gtk_widget_get_style_context(GTK_WIDGET(instance->profile_bigger)),
        class_name[status]);
}

static void wrapper_notify_connection_cb(LupusMainHeaderBar *instance) {
    Tox_Connection connection = lupus_wrapper_get_connection(lupus_wrapper);

    if (connection != TOX_CONNECTION_NONE) {
        wrapper_notify_status_cb(instance);
        return;
    }

    gint static const offset[] = {
        offsetof(LupusMainHeaderBar, profile_popover_none),
        offsetof(LupusMainHeaderBar, profile_popover_away),
        offsetof(LupusMainHeaderBar, profile_popover_busy),
    };

    for (gsize i = 0, j = G_N_ELEMENTS(offset); i < j; ++i) {
        gtk_widget_set_sensitive(
            GTK_WIDGET(*(gpointer *)((gchar *)instance + offset[i])), TRUE);
    }

    remove_class_with_prefix(instance->profile, "profile--");
    remove_class_with_prefix(instance->profile_bigger, "profile--");

    gtk_style_context_add_class(
        gtk_widget_get_style_context(GTK_WIDGET(instance->profile)),
        "profile--offline");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(GTK_WIDGET(instance->profile_bigger)),
        "profile--offline");
}

static void toxid_cb() {
    static GtkWidget *label;
    static GtkWidget *box;
    static GtkWidget *dialog;

    if (!label) {
        gchar *address = lupus_wrapper_get_address(lupus_wrapper);
        label = g_object_new(GTK_TYPE_LABEL, "label", address, "selectable",
                             TRUE, NULL);

        gtk_widget_set_margin_top(label, TOXID_DIALOG_MARGIN);
        gtk_widget_set_margin_end(label, TOXID_DIALOG_MARGIN);
        gtk_widget_set_margin_bottom(label, TOXID_DIALOG_MARGIN);
        gtk_widget_set_margin_start(label, TOXID_DIALOG_MARGIN);
    }

    if (!box) {
        box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    }

    if (!dialog) {
        dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, "title",
                              "ToxID", "resizable", FALSE, NULL);

        GtkContainer *container = GTK_CONTAINER(dialog);
        GtkWidget *child = gtk_container_get_children(container)->data;
        gtk_container_remove(container, child);
        gtk_container_add(container, box);

        gtk_widget_show_all(dialog);
    }

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
}

static void change_status_cb(GtkMenuItem *item, LupusMainHeaderBar *instance) {
    if (item == instance->profile_popover_none) {
        lupus_wrapper_set_status(lupus_wrapper, TOX_USER_STATUS_NONE);
    } else if (item == instance->profile_popover_away) {
        lupus_wrapper_set_status(lupus_wrapper, TOX_USER_STATUS_AWAY);
    } else {
        lupus_wrapper_set_status(lupus_wrapper, TOX_USER_STATUS_BUSY);
    }
}

static void init_profile_popover(LupusMainHeaderBar *instance) {
    GtkMenuItem **item[] = {
        &instance->profile_popover_none, &instance->profile_popover_away,
        &instance->profile_popover_busy, &instance->profile_popover_toxid};
    gchar *svg[] = {
        LUPUS_RESOURCES "/status_none.svg", LUPUS_RESOURCES "/status_away.svg",
        LUPUS_RESOURCES "/status_busy.svg", LUPUS_RESOURCES "/biometric.svg"};
    gchar *label[] = {"Online", "Away", "Busy", "ToxID"};

    for (gsize i = 0, j = G_N_ELEMENTS(label); i < j; ++i) {
        GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
        gtk_box_pack_start(box, gtk_image_new_from_resource(svg[i]), FALSE,
                           TRUE, 0);
        gtk_box_pack_start(box, gtk_label_new(label[i]), TRUE, TRUE, 0);

        *item[i] = GTK_MENU_ITEM(gtk_menu_item_new());
        gtk_container_add(GTK_CONTAINER(*item[i]), GTK_WIDGET(box));

        gtk_menu_shell_append(GTK_MENU_SHELL(instance->profile_popover),
                              GTK_WIDGET(*item[i]));

        if (i == (j - 2)) {
            gtk_menu_shell_append(GTK_MENU_SHELL(instance->profile_popover),
                                  gtk_separator_menu_item_new());
        }

        gpointer cb = (i == (j - 1) ? toxid_cb : change_status_cb);
        g_signal_connect(*item[i], "activate", cb, instance);
    }

    gtk_widget_set_sensitive(GTK_WIDGET(instance->profile_popover_none), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(instance->profile_popover_away), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(instance->profile_popover_busy), FALSE);

    gtk_widget_show_all(GTK_WIDGET(instance->profile_popover));
}

static void menu_popover_about_activate_cb() {
    static gchar const *authors[] = {"Ogromny", NULL};

    gtk_show_about_dialog(
        NULL, "authors", authors, "license_type", GTK_LICENSE_MIT_X11, "logo",
        gdk_pixbuf_new_from_resource(LUPUS_RESOURCES "/lupus.svg", NULL),
        "program-name", "Lupus", "version", LUPUS_VERSION, "website",
        "https://github.com/LupusApp/Lupus", "website-label", "Github",
        "wrap-license", TRUE, NULL);
}

static void init_menu_popover(LupusMainHeaderBar *instance) {
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_pack_start(
        box, gtk_image_new_from_icon_name("help-about", GTK_ICON_SIZE_MENU),
        FALSE, TRUE, 0);
    gtk_box_pack_start(box, gtk_label_new("About"), TRUE, TRUE, 0);

    GtkMenuItem *about_menu = GTK_MENU_ITEM(gtk_menu_item_new());
    gtk_container_add(GTK_CONTAINER(about_menu), GTK_WIDGET(box));

    gtk_menu_shell_append(GTK_MENU_SHELL(instance->menu_popover),
                          GTK_WIDGET(about_menu));

    gtk_widget_show_all(GTK_WIDGET(instance->menu_popover));

    g_signal_connect_swapped(about_menu, "activate",
                             G_CALLBACK(menu_popover_about_activate_cb),
                             instance);
}

static void active_chat_friend_notify_name_cb(LupusMainHeaderBar *instance) {
    LupusWrapperFriend *friend = lupus_wrapper_get_friend(
        lupus_wrapper, lupus_wrapper_get_active_chat_friend(lupus_wrapper));

    gtk_header_bar_set_title(instance->right_headerbar,
                             lupus_wrapperfriend_get_name(friend));
}

static void
active_chat_friend_notify_status_message_cb(LupusMainHeaderBar *instance) {
    LupusWrapperFriend *friend = lupus_wrapper_get_friend(
        lupus_wrapper, lupus_wrapper_get_active_chat_friend(lupus_wrapper));

    gtk_header_bar_set_subtitle(instance->right_headerbar,
                                lupus_wrapperfriend_get_status_message(friend));
}

static void wrapper_notify_active_chat_friend_cb(LupusMainHeaderBar *instance) {
    LupusWrapperFriend *friend = lupus_wrapper_get_friend(
        lupus_wrapper, lupus_wrapper_get_active_chat_friend(lupus_wrapper));

    gtk_header_bar_set_title(instance->right_headerbar,
                             lupus_wrapperfriend_get_name(friend));
    gtk_header_bar_set_subtitle(instance->right_headerbar,
                                lupus_wrapperfriend_get_status_message(friend));

    if (instance->active_chat_friend_notify_name_handler_id) {
        g_signal_handler_disconnect(
            instance, instance->active_chat_friend_notify_name_handler_id);
    }
    g_signal_connect_swapped(friend, "notify::name",
                             G_CALLBACK(active_chat_friend_notify_name_cb),
                             instance);

    if (instance->active_chat_friend_notify_status_message_handler_id) {
        g_signal_handler_disconnect(
            instance,
            instance->active_chat_friend_notify_status_message_handler_id);
    }
    g_signal_connect_swapped(
        friend, "notify::status-message",
        G_CALLBACK(active_chat_friend_notify_status_message_cb), instance);
}

static void file_chooser_dialog_update_preview_cb(GtkFileChooser *chooser,
                                                  GtkWidget *preview) {
    gchar *filename = gtk_file_chooser_get_preview_filename(chooser);
    if (!filename) {
        return;
    }

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(
        filename, FILE_CHOOSER_DIALOG_PREVIEW_SIZE,
        FILE_CHOOSER_DIALOG_PREVIEW_SIZE, NULL);
    g_free(filename);

    gtk_image_set_from_pixbuf(GTK_IMAGE(preview), pixbuf);

    if (pixbuf) {
        g_object_unref(pixbuf);
    }

    gtk_file_chooser_set_preview_widget_active(chooser, pixbuf != NULL);
}

static void profile_bigger_clicked_cb(LupusMainHeaderBar *instance) {
    static GtkFileChooser *chooser;

    if (!chooser) {
        chooser = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(
            "Select avatar",
            GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(instance))),
            GTK_FILE_CHOOSER_ACTION_OPEN, "Select", GTK_RESPONSE_ACCEPT,
            "Cancel", GTK_RESPONSE_CANCEL, NULL));

        GtkFileFilter *filter = gtk_file_filter_new();
        gtk_file_filter_add_mime_type(filter, "image/png");
        gtk_file_chooser_set_filter(chooser, filter);

        GtkWidget *preview = gtk_image_new();
        gtk_file_chooser_set_preview_widget(chooser, preview);

        g_signal_connect(chooser, "update-preview",
                         G_CALLBACK(file_chooser_dialog_update_preview_cb),
                         preview);
    }

    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename = gtk_file_chooser_get_filename(chooser);

        /*
         * Check file size
         */
        struct stat file_stat;
        g_stat(filename, &file_stat);

        if (file_stat.st_size > AVATAR_MAX_FILE_SIZE) {
            lupus_error("Avatar maximum size is <b>%d</b>.",
                        AVATAR_MAX_FILE_SIZE);
            goto end;
        }

        /*
         * Create avatars directory if not exist
         */
        gchar *avatars_directory = g_strconcat(LUPUS_TOX_DIR, "avatars/", NULL);
        if (!g_file_test(avatars_directory, G_FILE_TEST_IS_DIR)) {
            if (g_mkdir(avatars_directory, 755) != 0) { // NOLINT
                lupus_error("Cannot create avatars directory\n<b>%s</b>",
                            avatars_directory);
                g_free(avatars_directory);
                goto end;
            }
        }

        /*
         * Check if an avatar is already set, and if so, if it's the same
         */
        gchar *avatar_filename = g_strconcat(
            avatars_directory, lupus_wrapper_get_public_key(lupus_wrapper),
            ".png", NULL);
        g_free(avatars_directory);
        if (g_file_test(avatar_filename, G_FILE_TEST_EXISTS)) {
            gsize length = 0;
            gchar *contents = NULL;
            g_file_get_contents(filename, &contents, &length, NULL);

            gchar new_avatar_hash[TOX_HASH_LENGTH];
            tox_hash((guint8 *)new_avatar_hash, (guint8 *)contents, length);
            g_free(contents);

            gchar new_avatar_hash_hex[TOX_HASH_LENGTH * 2 + 1];
            sodium_bin2hex(new_avatar_hash_hex, sizeof(new_avatar_hash_hex),
                           (guint8 *)new_avatar_hash, sizeof(new_avatar_hash));

            if (g_strcmp0(new_avatar_hash_hex,
                          lupus_wrapper_get_avatar_hash(lupus_wrapper)) == 0) {
                lupus_error("This avatar is already set.");
                g_free(avatar_filename);
                goto end;
            }
        }

        /*
         * Square image and save it
         */
        cairo_surface_t *image = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, AVATAR_SIZE, AVATAR_SIZE);

        cairo_t *cr = cairo_create(image);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);

        cairo_surface_t *avatar = cairo_image_surface_create_from_png(filename);
        gint width = cairo_image_surface_get_width(avatar);
        gint height = cairo_image_surface_get_height(avatar);
        gfloat x = (AVATAR_SIZE - width) / 2.0;  // NOLINT
        gfloat y = (AVATAR_SIZE - height) / 2.0; // NOLINT
        cairo_set_source_surface(cr, avatar, x, y);
        cairo_paint(cr);
        cairo_surface_destroy(avatar);

        cairo_surface_flush(image);
        cairo_destroy(cr);

        cairo_surface_write_to_png(image, avatar_filename);
        cairo_surface_destroy(image);
        g_free(avatar_filename);

        /*
         * And finally, set avatar hash
         */
        lupus_wrapper_set_avatar_hash(lupus_wrapper);
    }

end:
    gtk_widget_hide(GTK_WIDGET(chooser));
}

static void wrapper_notify_avatar_hash_cb(LupusMainHeaderBar *instance) {
    gchar *avatar_hash = lupus_wrapper_get_avatar_hash(lupus_wrapper);
    gchar *public_key = lupus_wrapper_get_public_key(lupus_wrapper);

    gchar *filename =
        g_strconcat(LUPUS_TOX_DIR, "avatars/", public_key, ".png", NULL);

    if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
        g_free(filename);
        return;
    }

    gsize contents_length = 0;
    gchar *contents = NULL;
    g_file_get_contents(filename, &contents, &contents_length, NULL);

    guint8 hash[TOX_HASH_LENGTH];
    tox_hash(hash, (guint8 *)contents, contents_length);
    g_free(contents);

    gchar hash_hex[TOX_HASH_LENGTH * 2 + 1];
    sodium_bin2hex(hash_hex, sizeof(hash_hex), hash, sizeof(hash));

    if (g_strcmp0(hash_hex, avatar_hash)) {
        lupus_error("Avatar hash don't match.");
        goto end;
    }

    gtk_image_set_from_pixbuf(
        instance->profile_image,
        gdk_pixbuf_new_from_file_at_size(filename, AVATAR_SIZE_MINI,
                                         AVATAR_SIZE_MINI, NULL));
    gtk_image_set_from_pixbuf(instance->profile_bigger_image,
                              gdk_pixbuf_new_from_file_at_size(
                                  filename, AVATAR_SIZE, AVATAR_SIZE, NULL));
end:
    g_free(filename);
}

static void lupus_mainheaderbar_constructed(GObject *object) {
    LupusMainHeaderBar *instance = LUPUS_MAINHEADERBAR(object);

    instance->name = lupus_editablelabel_new(
        lupus_wrapper_get_name(lupus_wrapper), tox_max_name_length());
    instance->status_message =
        lupus_editablelabel_new(lupus_wrapper_get_status_message(lupus_wrapper),
                                tox_max_status_message_length());

    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->name), FALSE, FALSE,
                       0);
    gtk_box_pack_start(instance->vbox, GTK_WIDGET(instance->status_message),
                       FALSE, FALSE, 0);
    gtk_box_reorder_child(instance->vbox, GTK_WIDGET(instance->name), 0);

    g_signal_connect(instance->name, "submit", G_CALLBACK(name_submit_cb),
                     NULL);
    g_signal_connect(instance->status_message, "submit",
                     G_CALLBACK(status_message_submit_cb), NULL);
    g_signal_connect_swapped(instance->profile_bigger, "clicked",
                             G_CALLBACK(profile_bigger_clicked_cb), instance);

    instance->active_chat_friend_notify_name_handler_id = 0;
    instance->active_chat_friend_notify_status_message_handler_id = 0;

    wrapper_notify_avatar_hash_cb(instance);

    G_OBJECT_CLASS(lupus_mainheaderbar_parent_class) // NOLINT
        ->constructed(object);
}

static void lupus_mainheaderbar_class_init(LupusMainHeaderBarClass *class) {
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    gtk_widget_class_set_template_from_resource(widget_class, LUPUS_RESOURCES
                                                "/mainheaderbar.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         left_headerbar);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         right_headerbar);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         profile);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         profile_image);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         profile_popover);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         popover);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         profile_bigger);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         profile_bigger_image);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         vbox);
    gtk_widget_class_bind_template_child(widget_class, LupusMainHeaderBar,
                                         menu_popover);

    G_OBJECT_CLASS(class)->constructed = // NOLINT
        lupus_mainheaderbar_constructed;
}

static void lupus_mainheaderbar_init(LupusMainHeaderBar *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    init_profile_popover(instance);
    init_menu_popover(instance);

    g_signal_connect_swapped(lupus_wrapper, "notify::name",
                             G_CALLBACK(wrapper_notify_name_cb), instance);
    g_signal_connect_swapped(lupus_wrapper, "notify::status_message",
                             G_CALLBACK(wrapper_notify_status_message_cb),
                             instance);
    g_signal_connect_swapped(lupus_wrapper, "notify::status",
                             G_CALLBACK(wrapper_notify_status_cb), instance);
    g_signal_connect_swapped(lupus_wrapper, "notify::connection",
                             G_CALLBACK(wrapper_notify_connection_cb),
                             instance);
    g_signal_connect_swapped(lupus_wrapper, "notify::avatar-hash",
                             G_CALLBACK(wrapper_notify_avatar_hash_cb),
                             instance);

    g_signal_connect_swapped(instance->profile, "clicked",
                             G_CALLBACK(gtk_popover_popup), instance->popover);
    g_signal_connect_swapped(instance->profile, "button-press-event",
                             G_CALLBACK(profile_button_press_event), instance);
    g_signal_connect_swapped(lupus_wrapper, "notify::active-chat-friend",
                             G_CALLBACK(wrapper_notify_active_chat_friend_cb),
                             instance);
}

LupusMainHeaderBar *lupus_mainheaderbar_new(void) {
    return g_object_new(LUPUS_TYPE_MAINHEADERBAR, NULL);
}