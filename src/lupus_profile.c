#include "../include/lupus_profile.h"
#include "../include/lupus_editablelabel.h"

struct _LupusProfile {
    GtkBox parent_instance;

    LupusObjectSelf *object_self;

    GtkBox *vbox;

    GtkEventBox *avatar_event_box;
    GtkImage *avatar;
    LupusEditableLabel *name, *status_message;
};

G_DEFINE_TYPE(LupusProfile, lupus_profile, GTK_TYPE_BOX)

typedef enum {
    PROP_OBJECTSELF = 1,
    N_PROPERTIES,
} LupusProfileProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

#define AVATAR_PREVIEW_SIZE 128

static void object_self_avatar_pixbuf_cb(LupusProfile *instance)
{
    GdkPixbuf *avatar_pixbuf;
    g_object_get(instance->object_self, "avatar-pixbuf", &avatar_pixbuf, NULL);

    gtk_image_set_from_pixbuf(instance->avatar, avatar_pixbuf);
}

static gboolean name_submitted_cb(LupusProfile *instance, gchar *name)
{
    g_object_set(instance->object_self, "name", name, NULL);
    return TRUE;
}

static gboolean status_message_submitted_cb(LupusProfile *instance, gchar *status_message)
{
    g_object_set(instance->object_self, "status-message", status_message, NULL);
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
    if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
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
            g_object_set(instance->object_self, "avatar-filename", filename, NULL);
        }

        gtk_widget_hide(GTK_WIDGET(file_chooser));
    }

    return FALSE;
}

static void lupus_profile_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    LupusProfile *instance = LUPUS_PROFILE(object);

    switch ((LupusProfileProperty)property_id) {
    case PROP_OBJECTSELF:
        g_value_set_pointer(value, instance->object_self);
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
        instance->object_self = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_profile_constructed(GObject *object)
{
    LupusProfile *instance = LUPUS_PROFILE(object);

    gchar *object_self_name, *object_self_status_message;
    g_object_get(instance->object_self, "name", &object_self_name, "status-message", &object_self_status_message, NULL);
    instance->name = lupus_editablelabel_new(object_self_name, tox_max_name_length());
    instance->status_message = lupus_editablelabel_new(object_self_status_message, tox_max_status_message_length());

    instance->vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    GtkWidget *name = GTK_WIDGET(instance->name);
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *status_message = GTK_WIDGET(instance->status_message);
    gtk_box_pack_end(instance->vbox, status_message, FALSE, TRUE, 0);
    gtk_box_pack_end(instance->vbox, separator, FALSE, TRUE, 0);
    gtk_box_pack_end(instance->vbox, name, FALSE, TRUE, 0);

    GdkPixbuf *object_self_avatar_pixbuf;
    g_object_get(instance->object_self, "avatar-pixbuf", &object_self_avatar_pixbuf, NULL);
    instance->avatar = GTK_IMAGE(gtk_image_new_from_pixbuf(object_self_avatar_pixbuf));

    instance->avatar_event_box = g_object_new(GTK_TYPE_EVENT_BOX, "child", instance->avatar, NULL);

    GtkBox *box = GTK_BOX(instance);
    GtkWidget *vbox = GTK_WIDGET(instance->vbox);
    GtkWidget *avatar_event_box = GTK_WIDGET(instance->avatar_event_box);
    gtk_box_pack_end(box, avatar_event_box, FALSE, TRUE, 0);
    gtk_box_pack_end(box, vbox, TRUE, TRUE, 0);

    GtkWidget *widget = GTK_WIDGET(box);
    gtk_widget_show_all(widget);

    g_signal_connect_swapped(instance->name, "submit", G_CALLBACK(name_submitted_cb), instance);
    g_signal_connect_swapped(instance->status_message, "submit", G_CALLBACK(status_message_submitted_cb), instance);
    g_signal_connect_swapped(instance->avatar_event_box, "button-press-event", G_CALLBACK(avatar_button_press_event_cb),
                             instance);
    g_signal_connect_swapped(instance->object_self, "notify::avatar-pixbuf", G_CALLBACK(object_self_avatar_pixbuf_cb),
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
        g_param_spec_pointer("object-self", NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void lupus_profile_init(LupusProfile *instance) {}

LupusProfile *lupus_profile_new(LupusObjectSelf *object_self)
{
    return g_object_new(LUPUS_TYPE_PROFILE, "object-self", object_self, "orientation", GTK_ORIENTATION_HORIZONTAL,
                        "border-width", 5, "spacing", 5, NULL);
}