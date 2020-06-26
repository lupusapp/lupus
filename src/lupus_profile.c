#include "../include/lupus_profile.h"
#include "../include/lupus_editablelabel.h"
#include "include/lupus.h"
#include "include/lupus_objectself.h"
#include <tox/tox.h>

struct _LupusProfile {
    GtkBox parent_instance;

    LupusObjectSelf *objectself;

    GtkBox *vbox;

    GtkEventBox *avatar_event_box;
    GtkImage *avatar;
    LupusEditableLabel *name, *status_message;
    GtkMenu *popover;
};

G_DEFINE_TYPE(LupusProfile, lupus_profile, GTK_TYPE_BOX)

typedef enum {
    PROP_OBJECTSELF = 1,
    N_PROPERTIES,
} LupusProfileProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

#define AVATAR_PREVIEW_SIZE 128

static void objectself_user_status_cb(LupusProfile *instance)
{
    Tox_User_Status status;
    g_object_get(instance->objectself, "user-status", &status, NULL);
    Tox_Connection connection;
    g_object_get(instance->objectself, "connection", &connection, NULL);

    if (connection == TOX_CONNECTION_NONE) {
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

static void objectself_connection_cb(LupusProfile *instance)
{
    Tox_Connection connection = TOX_CONNECTION_NONE;
    g_object_get(instance->objectself, "connection", &connection, NULL);

    if (connection != TOX_CONNECTION_NONE) {
        objectself_user_status_cb(instance);
        return;
    }

    widget_remove_classes_with_prefix(instance->avatar, "profile--");
}

static void objectself_avatar_pixbuf_cb(LupusProfile *instance)
{
    GdkPixbuf *avatar_pixbuf;
    g_object_get(instance->objectself, "avatar-pixbuf", &avatar_pixbuf, NULL);

    gtk_image_set_from_pixbuf(instance->avatar, avatar_pixbuf);
}

static gboolean name_submitted_cb(LupusProfile *instance, gchar *name)
{
    g_object_set(instance->objectself, "name", name, NULL);
    return TRUE;
}

static gboolean status_message_submitted_cb(LupusProfile *instance, gchar *status_message)
{
    g_object_set(instance->objectself, "status-message", status_message, NULL);
    return TRUE;
}

static void update_preview_cb(GtkFileChooser *file_chooser, GtkImage *preview)
{
    gchar *filename = gtk_file_chooser_get_filename(file_chooser);
    if (!filename) {
        return;
    }

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(filename, AVATAR_PREVIEW_SIZE, AVATAR_PREVIEW_SIZE, NULL);
    g_free(filename);

    if (pixbuf) {
        gtk_image_set_from_pixbuf(preview, pixbuf);
        g_object_unref(pixbuf);
    }

    gtk_file_chooser_set_preview_widget_active(file_chooser, !!pixbuf);
}

static gboolean avatar_button_press_event_cb(LupusProfile *instance, GdkEvent *event)
{
    if (event->type != GDK_BUTTON_PRESS) {
        return false;
    }

    if (event->button.button == 1) {
        static GtkFileChooser *file_chooser;

        if (!file_chooser) {
            GtkWindow *window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(instance)));

            file_chooser = GTK_FILE_CHOOSER(
                gtk_file_chooser_dialog_new("Select avatar", window, GTK_FILE_CHOOSER_ACTION_OPEN, "Select",
                                            GTK_RESPONSE_ACCEPT, "Cancel", GTK_RESPONSE_CANCEL, NULL));

            GtkFileFilter *filter = gtk_file_filter_new();
            gtk_file_filter_add_mime_type(filter, "image/png");
            gtk_file_chooser_set_filter(file_chooser, filter);

            GtkWidget *preview = gtk_image_new();
            gtk_file_chooser_set_preview_widget(file_chooser, preview);

            g_signal_connect(file_chooser, "update-preview", G_CALLBACK(update_preview_cb), preview);
        }

        if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
            gchar *filename = gtk_file_chooser_get_filename(file_chooser);
            g_object_set(instance->objectself, "avatar-filename", filename, NULL);
        }

        gtk_widget_hide(GTK_WIDGET(file_chooser));
    } else if (event->button.button == 3) {
        gtk_menu_popup_at_pointer(instance->popover, event);
    }

    return FALSE;
}

static void popover_status_activate_cb(GtkMenuItem *item, LupusProfile *instance)
{
    GtkWidget *box = gtk_bin_get_child(GTK_BIN(item));
    GList *children = gtk_container_get_children(GTK_CONTAINER(box));
    gchar const *text = gtk_label_get_text(GTK_LABEL(g_list_nth_data(children, 1)));
    g_list_free(children);

    Tox_User_Status status = TOX_USER_STATUS_NONE;
    if (!g_strcmp0(text, "Away")) {
        status = TOX_USER_STATUS_AWAY;
    } else if (!g_strcmp0(text, "Busy")) {
        status = TOX_USER_STATUS_BUSY;
    }

    g_object_set(instance->objectself, "user-status", status, NULL);
}

static void popover_myid_activate_cb(LupusProfile *instance)
{
    // Cannot make widgets static, because address can be changed due to nospam or pk

    gchar *address;
    g_object_get(instance->objectself, "address", &address, NULL);

    GtkWidget *dialog = g_object_new(GTK_TYPE_DIALOG, "use-header-bar", TRUE, "title", "My ID", "resizable", FALSE,
                                     "border-width", 5, NULL);

    GtkBox *box = GTK_BOX(gtk_bin_get_child(GTK_BIN(dialog)));
    GtkWidget *label = g_object_new(GTK_TYPE_LABEL, "label", address, "selectable", TRUE, NULL);
    gtk_box_pack_start(box, label, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
}

static void construct_popover(LupusProfile *instance)
{
    instance->popover = GTK_MENU(gtk_menu_new());

    gchar *resource[] = {
        LUPUS_RESOURCES "/status_none.svg",
        LUPUS_RESOURCES "/status_away.svg",
        LUPUS_RESOURCES "/status_busy.svg",
        LUPUS_RESOURCES "/biometric.svg",
    };
    gchar *label[] = {
        "Online",
        "Away",
        "Busy",
        "My ID",
    };

    for (gsize i = 0, j = G_N_ELEMENTS(resource); i < j; ++i) {
        GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
        gtk_box_pack_start(box, gtk_image_new_from_resource(resource[i]), FALSE, TRUE, 0);
        gtk_box_pack_start(box, gtk_label_new(label[i]), TRUE, TRUE, 0);

        GtkWidget *item = gtk_menu_item_new();
        gtk_container_add(GTK_CONTAINER(item), GTK_WIDGET(box));

        if (i == (j - 1)) {
            gtk_menu_shell_append(GTK_MENU_SHELL(instance->popover), gtk_separator_menu_item_new());

            g_signal_connect_swapped(item, "activate", G_CALLBACK(popover_myid_activate_cb), instance);
        } else {
            g_signal_connect(item, "activate", G_CALLBACK(popover_status_activate_cb), instance);
        }

        gtk_menu_shell_append(GTK_MENU_SHELL(instance->popover), item);
    }

    gtk_widget_show_all(GTK_WIDGET(instance->popover));
}

static void lupus_profile_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    LupusProfile *instance = LUPUS_PROFILE(object);

    switch ((LupusProfileProperty)property_id) {
    case PROP_OBJECTSELF:
        g_value_set_pointer(value, instance->objectself);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_profile_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusProfile *instance = LUPUS_PROFILE(object);

    switch ((LupusProfileProperty)property_id) {
    case PROP_OBJECTSELF:
        instance->objectself = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_profile_constructed(GObject *object)
{
    LupusProfile *instance = LUPUS_PROFILE(object);

    gchar *objectself_name, *objectself_status_message;
    g_object_get(instance->objectself, "name", &objectself_name, "status-message", &objectself_status_message, NULL);
    instance->name = lupus_editablelabel_new(objectself_name, tox_max_name_length());
    instance->status_message = lupus_editablelabel_new(objectself_status_message, tox_max_status_message_length());

    instance->vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    GtkWidget *name = GTK_WIDGET(instance->name);
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *status_message = GTK_WIDGET(instance->status_message);
    gtk_box_pack_end(instance->vbox, status_message, FALSE, TRUE, 0);
    gtk_box_pack_end(instance->vbox, separator, FALSE, TRUE, 0);
    gtk_box_pack_end(instance->vbox, name, FALSE, TRUE, 0);

    GdkPixbuf *objectself_avatar_pixbuf;
    g_object_get(instance->objectself, "avatar-pixbuf", &objectself_avatar_pixbuf, NULL);
    if (!objectself_avatar_pixbuf) {
        // TODO: write default avatar ?
        objectself_avatar_pixbuf =
            gdk_pixbuf_new_from_resource_at_scale(LUPUS_RESOURCES "/lupus.svg", AVATAR_SIZE, AVATAR_SIZE, TRUE, NULL);
    }
    instance->avatar = GTK_IMAGE(gtk_image_new_from_pixbuf(objectself_avatar_pixbuf));
    widget_add_class(instance->avatar, "profile");

    instance->avatar_event_box =
        g_object_new(GTK_TYPE_EVENT_BOX, "child", instance->avatar, "valign", GTK_ALIGN_CENTER, NULL);

    GtkBox *box = GTK_BOX(instance);
    GtkWidget *vbox = GTK_WIDGET(instance->vbox);
    GtkWidget *avatar_event_box = GTK_WIDGET(instance->avatar_event_box);
    gtk_box_pack_end(box, avatar_event_box, FALSE, TRUE, 0);
    gtk_box_pack_end(box, vbox, TRUE, TRUE, 0);

    GtkWidget *widget = GTK_WIDGET(box);
    gtk_widget_show_all(widget);

    construct_popover(instance);

    g_signal_connect_swapped(instance->name, "submit", G_CALLBACK(name_submitted_cb), instance);
    g_signal_connect_swapped(instance->status_message, "submit", G_CALLBACK(status_message_submitted_cb), instance);
    g_signal_connect_swapped(instance->avatar_event_box, "button-press-event", G_CALLBACK(avatar_button_press_event_cb),
                             instance);
    g_signal_connect_swapped(instance->objectself, "notify::avatar-pixbuf", G_CALLBACK(objectself_avatar_pixbuf_cb),
                             instance);
    g_signal_connect_swapped(instance->objectself, "notify::connection", G_CALLBACK(objectself_connection_cb),
                             instance);
    g_signal_connect_swapped(instance->objectself, "notify::user-status", G_CALLBACK(objectself_user_status_cb),
                             instance);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_profile_parent_class);
    object_class->constructed(object);
}

static void lupus_profile_class_init(LupusProfileClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS(class);

    object_class->constructed = lupus_profile_constructed;
    object_class->set_property = lupus_profile_set_property;
    object_class->get_property = lupus_profile_get_property;

    obj_properties[PROP_OBJECTSELF] =
        g_param_spec_pointer("objectself", NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_profile_init(LupusProfile *instance) {}

LupusProfile *lupus_profile_new(LupusObjectSelf *objectself)
{
    return g_object_new(LUPUS_TYPE_PROFILE, "objectself", objectself, "orientation", GTK_ORIENTATION_HORIZONTAL,
                        "border-width", 5, "spacing", 5, NULL);
}

