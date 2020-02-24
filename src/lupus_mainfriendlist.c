#include "../include/lupus_mainfriendlist.h"
#include "../include/lupus.h"
#include "../include/lupus_mainfriend.h"
#include "toxcore/tox.h"
#include <sodium.h>

struct _LupusMainFriendList {
    GtkEventBox parent_instance;

    Tox const *tox;
    LupusMain *main;

    GtkBox *box;
    GtkMenu *list_menu;
    GtkMenuItem *list_menu_addfriend;
};

G_DEFINE_TYPE(LupusMainFriendList, lupus_mainfriendlist, GTK_TYPE_EVENT_BOX)

enum { PROP_TOX = 1, PROP_MAIN, N_PROPERTIES };

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};
static GHashTable *mainfriends;

#define ADDFRIEND_DIALOG_WIDTH 350
#define ADDFRIEND_DIALOG_HEIGHT 200
#define ADDFRIEND_DIALOG_MARGIN 5
#define SEPARATOR_MARGIN 5

static void refresh(LupusMainFriendList *instance) {
    guint32 friends[tox_self_get_friend_list_size(instance->tox)];
    tox_self_get_friend_list(instance->tox, friends);

    GList *children = gtk_container_get_children(GTK_CONTAINER(instance->box));
    for (GList *child = children; child != NULL; child = g_list_next(child)) {
        gtk_widget_destroy(GTK_WIDGET(child->data));
    }
    g_list_free(children);

    g_hash_table_remove_all(mainfriends);

    for (gsize i = 0, j = G_N_ELEMENTS(friends); i < j; ++i) {
        LupusMainFriend *mainfriend =
            lupus_mainfriend_new(instance->tox, instance->main, friends[i]);
        g_hash_table_insert(mainfriends, GUINT_TO_POINTER(friends[i]),
                            mainfriend);
        gtk_box_pack_start(instance->box, GTK_WIDGET(mainfriend), FALSE, TRUE,
                           0);

        GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_set_margin_start(GTK_WIDGET(separator), SEPARATOR_MARGIN);
        gtk_widget_set_margin_end(GTK_WIDGET(separator), SEPARATOR_MARGIN);
        gtk_box_pack_start(instance->box, separator, FALSE, TRUE, 0);
    }

    gtk_widget_show_all(GTK_WIDGET(instance->box));
}

// NOLINTNEXTLINE
static void addfriend_cb(LupusMainFriendList *instance) {
    GtkDialog *dialog =
        GTK_DIALOG(g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, NULL));
    gtk_dialog_add_button(dialog, "Add", 1);

    gtk_header_bar_set_title(
        GTK_HEADER_BAR(gtk_dialog_get_header_bar(GTK_DIALOG(dialog))),
        "Add friend");

    GtkBox *box =
        GTK_BOX(gtk_container_get_children(GTK_CONTAINER(dialog))->data);
    gtk_widget_set_margin_top(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_end(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_bottom(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);
    gtk_widget_set_margin_start(GTK_WIDGET(box), ADDFRIEND_DIALOG_MARGIN);

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
            lupus_error(NULL, "Please enter an address.");
            goto end;
        }

        guchar address_bin[TOX_ADDRESS_SIZE];
        gchar const *message = gtk_entry_get_text(friend_message);

        sodium_hex2bin(address_bin, sizeof(address_bin), address_hex,
                       strlen(address_hex), NULL, NULL, NULL);

        enum TOX_ERR_FRIEND_ADD tox_err_friend_add;
        tox_friend_add((Tox *)instance->tox, address_bin, (guint8 *)message,
                       strlen(message), &tox_err_friend_add);

        switch (tox_err_friend_add) {
        case TOX_ERR_FRIEND_ADD_OK:
            lupus_success(instance->main, "Friend successfully added.");
            break;
        case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
            lupus_error(instance->main, "Please enter a message.");
            goto end;
        case TOX_ERR_FRIEND_ADD_OWN_KEY:
            lupus_error(instance->main, "It's your personal address.");
            goto end;
        case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
            lupus_error(instance->main, "Friend request already sent.");
            goto end;
        case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
            lupus_error(instance->main, "Wrong friend address.");
            goto end;
        default:
            lupus_error(instance->main, "Cannot add friend.");
            goto end;
        }

        /* FIXME: return value from save */
        g_signal_emit_by_name(instance->main, "save", NULL);

        refresh(instance);
    }

end:
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static gboolean click_cb(LupusMainFriendList *instance, GdkEvent *event) {
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 3) {
        gtk_menu_popup_at_pointer(instance->list_menu, event);
    }
    return FALSE;
}

static void lupus_mainfriendlist_set_property(LupusMainFriendList *instance,
                                              guint property_id,
                                              GValue const *value,
                                              GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        instance->tox = g_value_get_pointer(value);
        break;
    case PROP_MAIN:
        instance->main = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void lupus_mainfriendlist_get_property(LupusMainFriendList *instance,
                                              guint property_id, GValue *value,
                                              GParamSpec *pspec) {
    switch (property_id) {
    case PROP_TOX:
        g_value_set_pointer(value, (gpointer)instance->tox);
        break;
    case PROP_MAIN:
        g_value_set_pointer(value, (gpointer)instance->main);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec);
    }
}

static void friend_name_cb(Tox *tox, guint32 friend_number) { // NOLINT
    g_signal_emit_by_name(
        g_hash_table_lookup(mainfriends, GUINT_TO_POINTER(friend_number)),
        "update", UPDATE_NAME);
}

static void friend_status_message_cb(Tox *tox, // NOLINT
                                     guint32 friend_number) {
    g_signal_emit_by_name(
        g_hash_table_lookup(mainfriends, GUINT_TO_POINTER(friend_number)),
        "update", UPDATE_STATUS_MESSAGE);
}

static void friend_connection_status_cb(Tox *tox, // NOLINT
                                        guint32 friend_number) {
    g_signal_emit_by_name(
        g_hash_table_lookup(mainfriends, GUINT_TO_POINTER(friend_number)),
        "update", UPDATE_STATUS);
}

static void lupus_mainfriendlist_constructed(LupusMainFriendList *instance) {
    mainfriends = g_hash_table_new(NULL, NULL);

    tox_callback_friend_name((Tox *)instance->tox,
                             (tox_friend_name_cb *)friend_name_cb);
    tox_callback_friend_status_message(
        (Tox *)instance->tox,
        (tox_friend_status_message_cb *)friend_status_message_cb);
    tox_callback_friend_connection_status(
        (Tox *)instance->tox,
        (tox_friend_connection_status_cb *)friend_connection_status_cb);

    refresh(instance);
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

    G_OBJECT_CLASS(class)->set_property = // NOLINT
        lupus_mainfriendlist_set_property;
    G_OBJECT_CLASS(class)->get_property = // NOLINT
        lupus_mainfriendlist_get_property;
    G_OBJECT_CLASS(class)->constructed = // NOLINT
        lupus_mainfriendlist_constructed;

    obj_properties[PROP_TOX] = g_param_spec_pointer(
        "tox", "Tox", "Tox profile.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    obj_properties[PROP_MAIN] = g_param_spec_pointer(
        "main", "Main", "LupusMain parent.",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY); // NOLINT

    g_object_class_install_properties(G_OBJECT_CLASS(class), // NOLINT
                                      N_PROPERTIES, obj_properties);
}

static void lupus_mainfriendlist_init(LupusMainFriendList *instance) {
    gtk_widget_init_template(GTK_WIDGET(instance));

    g_signal_connect(instance, "button-press-event", G_CALLBACK(click_cb),
                     NULL);
    g_signal_connect_swapped(instance->list_menu_addfriend, "activate",
                             G_CALLBACK(addfriend_cb), instance);
}

LupusMainFriendList *lupus_mainfriendlist_new(Tox const *tox, LupusMain *main) {
    return g_object_new(LUPUS_TYPE_MAINFRIENDLIST, "tox", tox, "main", main,
                        NULL);
}