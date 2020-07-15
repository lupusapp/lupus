#pragma once

#define LUPUS_APPLICATION_ID "ru.ogromny.lupus"
#define LUPUS_RESOURCES      "/ru/ogromny/lupus"
#define LUPUS_VERSION        "0.9.9"

#define AVATAR_SIZE          48
#define AVATAR_FRIEND_SIZE   36
#define AVATAR_MAX_FILE_SIZE 65536

#define widget_add_class(w, class_name)                                  \
    do {                                                                 \
        GtkWidget *widget = GTK_WIDGET(w);                               \
        GtkStyleContext *context = gtk_widget_get_style_context(widget); \
        gtk_style_context_add_class(context, class_name);                \
    } while (0)

#define widget_remove_class(w, class_name)                               \
    do {                                                                 \
        GtkWidget *widget = GTK_WIDGET(w);                               \
        GtkStyleContext *context = gtk_widget_get_style_context(widget); \
        gtk_style_context_remove_class(context, class_name);             \
    } while (0)

#define widget_remove_classes_with_prefix(w, prefix)                     \
    do {                                                                 \
        GtkWidget *widget = GTK_WIDGET(w);                               \
        GtkStyleContext *context = gtk_widget_get_style_context(widget); \
        GList *classes = gtk_style_context_list_classes(context);        \
        for (GList *class = classes; class; class = class->next) {       \
            if (g_str_has_prefix(class->data, prefix)) {                 \
                gtk_style_context_remove_class(context, class->data);    \
            }                                                            \
        }                                                                \
        g_list_free(classes);                                            \
    } while (0)

#define lupus_error(...) lupus_message(GTK_MESSAGE_ERROR, "<b>Error</b>", __VA_ARGS__)

#define lupus_success(...) lupus_message(GTK_MESSAGE_INFO, "<b>Success</b>", __VA_ARGS__)

#define lupus_message(t, s, ...)                                                                            \
    do {                                                                                                    \
        GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_USE_HEADER_BAR, t, GTK_BUTTONS_CLOSE, NULL); \
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(d), s);                                            \
        gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(d), __VA_ARGS__);                     \
        gtk_dialog_run(GTK_DIALOG(d));                                                                      \
        gtk_widget_destroy(GTK_WIDGET(d));                                                                  \
    } while (0)

#define _CONCAT(x, y) x##y
#define CONCAT(x, y)  _CONCAT(x, y)

#define INSTANCE TN *instance

#define connect(obj, event, cb, data)         g_signal_connect(obj, event, G_CALLBACK(cb), data)
#define connect_swapped(obj, event, cb, data) g_signal_connect_swapped(obj, event, G_CALLBACK(cb), data)
#define disconnect(obj, id)                   g_signal_handler_disconnect(obj, id)

#define notify(obj, prop)               g_object_notify_by_pspec(G_OBJECT(obj), obj_properties[prop])
#define emit_by_pspec(obj, signal, ...) g_signal_emit(obj, signal, 0, __VA_ARGS__)
#define emit_by_name(obj, signal, ...)  g_signal_emit_by_name(obj, signal, __VA_ARGS__)

#define object_get_prop(object, prop_name, var, vartype) \
    vartype var;                                         \
    g_object_get(object, prop_name, &var, NULL);

#define object_set_prop(object, prop_name, value) g_object_set(object, prop_name, value, NULL);

#define finalize_header()                                \
    static void CONCAT(t_n, _finalize)(GObject * object) \
    {                                                    \
        TN *instance = T_N(object);

#define finalize_footer()                                                    \
    GObjectClass *object_class = G_OBJECT_CLASS(CONCAT(t_n, _parent_class)); \
    object_class->finalize(object);                                          \
    }

#define constructed_header()                                \
    static void CONCAT(t_n, _constructed)(GObject * object) \
    {                                                       \
        TN *instance = T_N(object);

#define constructed_footer()                                                 \
    GObjectClass *object_class = G_OBJECT_CLASS(CONCAT(t_n, _parent_class)); \
    object_class->constructed(object);                                       \
    }

#define declare_signals(...) \
    typedef enum {           \
        __VA_ARGS__,         \
        LAST_SIGNAL,         \
    } CONCAT(TN, Signal);    \
    static guint signals[LAST_SIGNAL]

#define define_signal(SIGNAL, name, TYPE, RETURN_TYPE, N_ARGS, ...) \
    signals[SIGNAL] =                                               \
        g_signal_new(name, TYPE, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, RETURN_TYPE, N_ARGS, __VA_ARGS__);

#define declare_properties(...) \
    typedef enum {              \
        PROP_0,                 \
        __VA_ARGS__,            \
        N_PROPERTIES,           \
    } CONCAT(TN, Property);     \
    static GParamSpec *obj_properties[N_PROPERTIES] = {NULL}

#define define_property(PROPERTY, type, name, ...) \
    obj_properties[PROPERTY] = g_param_spec_##type(name, NULL, NULL, __VA_ARGS__);

#define class_init() static void CONCAT(t_n, _class_init)(CONCAT(TN, Class) * class)
#define init()       static void CONCAT(t_n, _init)(CONCAT(TN, ) * instance)

#define set_property_header()                                                                        \
    static void CONCAT(t_n, _set_property)(GObject * object, guint property_id, GValue const *value, \
                                           GParamSpec *pspec)                                        \
    {                                                                                                \
        TN *instance = T_N(object);                                                                  \
        switch ((CONCAT(TN, Property))property_id) {

#define set_property_footer()                                            \
    default:                                                             \
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec); \
        }                                                                \
        }

#define get_property_header()                                                                                       \
    static void CONCAT(t_n, _get_property)(GObject * object, guint property_id, GValue * value, GParamSpec * pspec) \
    {                                                                                                               \
        TN *instance = T_N(object);                                                                                 \
        switch ((CONCAT(TN, Property))property_id) {

#define get_property_footer()                                            \
    default:                                                             \
        G_OBJECT_WARN_INVALID_PROPERTY_ID(instance, property_id, pspec); \
        }                                                                \
        }

extern char *LUPUS_TOX_DIR;
