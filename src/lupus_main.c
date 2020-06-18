#include "../include/lupus_main.h"
#include "../include/lupus.h"
#include "../include/lupus_editablelabel.h"
#include "../include/lupus_friend.h"
#include "../include/lupus_profile.h"
#include "include/lupus_objectfriend.h"

struct _LupusMain {
    GtkApplicationWindow parent_instance;

    LupusObjectSelf *object_self;
    LupusProfile *profile;

    GtkBox *sidebox;
    GtkScrolledWindow *sidebox_friends_scrolled_window;
    GtkBox *sidebox_friends_box;
};

G_DEFINE_TYPE(LupusMain, lupus_main, GTK_TYPE_APPLICATION_WINDOW)

typedef enum {
    PROP_OBJECTSELF = 1,
    N_PROPERTIES,
} LupusMainProperty;
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL};

static void construct_sidebox_friends(LupusMain *instance)
{
    GtkWidget *sidebox_friends_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    instance->sidebox_friends_scrolled_window = GTK_SCROLLED_WINDOW(sidebox_friends_scrolled_window);
    GtkWidget *sidebox_friends_box =
        g_object_new(GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 5, "spacing", 5, NULL);
    instance->sidebox_friends_box = GTK_BOX(sidebox_friends_box);
    gtk_container_add(GTK_CONTAINER(sidebox_friends_scrolled_window), sidebox_friends_box);
    gtk_box_pack_start(instance->sidebox, sidebox_friends_scrolled_window, TRUE, TRUE, 0);

    GHashTable *objectfriends;
    g_object_get(instance->object_self, "objectfriends", &objectfriends, NULL);
    if (!objectfriends) {
        return;
    }

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, objectfriends);
    gpointer value = NULL;

    while (g_hash_table_iter_next(&iter, NULL, &value)) {
        LupusObjectFriend *objectfriend = LUPUS_OBJECTFRIEND(value);
        LupusFriend *friend = lupus_friend_new(objectfriend);

        gtk_box_pack_start(instance->sidebox_friends_box, GTK_WIDGET(friend), FALSE, TRUE, 0);
        gtk_box_pack_start(instance->sidebox_friends_box, gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, TRUE,
                           0);
    }
}

static void lupus_main_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    LupusMain *instance = LUPUS_MAIN(object);

    switch ((LupusMainProperty)property_id) {
    case PROP_OBJECTSELF:
        g_value_set_pointer(value, instance->object_self);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_main_set_property(GObject *object, guint property_id, GValue const *value, GParamSpec *pspec)
{
    LupusMain *instance = LUPUS_MAIN(object);

    switch ((LupusMainProperty)property_id) {
    case PROP_OBJECTSELF:
        instance->object_self = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void lupus_main_constructed(GObject *object)
{
    LupusMain *instance = LUPUS_MAIN(object);

    instance->profile = lupus_profile_new(instance->object_self);
    GtkWidget *profile = GTK_WIDGET(instance->profile);
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(instance->sidebox, profile, FALSE, TRUE, 0);
    gtk_box_pack_start(instance->sidebox, separator, FALSE, TRUE, 0);

    construct_sidebox_friends(instance);

    GtkWidget *widget = GTK_WIDGET(instance);
    gtk_widget_show_all(widget);

    GObjectClass *object_class = G_OBJECT_CLASS(lupus_main_parent_class);
    object_class->constructed(object);
}

static void lupus_main_init(LupusMain *instance)
{
    GtkWidget *widget = GTK_WIDGET(instance);

    gtk_widget_init_template(widget);
}

static void lupus_main_class_init(LupusMainClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);

    gtk_widget_class_set_template_from_resource(widget_class, LUPUS_RESOURCES "/main.ui");
    gtk_widget_class_bind_template_child(widget_class, LupusMain, sidebox);

    GObjectClass *object_class = G_OBJECT_CLASS(class);
    object_class->constructed = lupus_main_constructed;
    object_class->set_property = lupus_main_set_property;
    object_class->get_property = lupus_main_get_property;

    obj_properties[PROP_OBJECTSELF] =
        g_param_spec_pointer("object-self", NULL, NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

LupusMain *lupus_main_new(GtkApplication *application, LupusObjectSelf *object_self)
{
    return g_object_new(LUPUS_TYPE_MAIN, "application", application, "object-self", object_self, NULL);
}

