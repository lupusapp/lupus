#ifndef __LUPUS_LUPUS_H__
#define __LUPUS_LUPUS_H__

#define LUPUS_APPLICATION_ID "ru.ogromny.lupus"
#define LUPUS_RESOURCES "/ru/ogromny/lupus"
#define LUPUS_VERSION "0.9.9"

#define AVATAR_SIZE 48

#define widget_add_class(w, class_name)                                                                                \
    do {                                                                                                               \
        GtkWidget *widget = GTK_WIDGET(w);                                                                             \
        GtkStyleContext *context = gtk_widget_get_style_context(widget);                                               \
        gtk_style_context_add_class(context, class_name);                                                              \
    } while (0)

#define widget_remove_class(w, class_name)                                                                             \
    do {                                                                                                               \
        GtkWidget *widget = GTK_WIDGET(w);                                                                             \
        GtkStyleContext *context = gtk_widget_get_style_context(widget);                                               \
        gtk_style_context_remove_class(context, class_name);                                                           \
    } while (0)

#define widget_remove_classes_with_prefix(w, prefix)                                                                   \
    do {                                                                                                               \
        GtkWidget *widget = GTK_WIDGET(w);                                                                             \
        GtkStyleContext *context = gtk_widget_get_style_context(widget);                                               \
        GList *classes = gtk_style_context_list_classes(context);                                                      \
        for (GList *class = classes; class; class = class->next) {                                                     \
            if (g_str_has_prefix(class->data, prefix)) {                                                               \
                gtk_style_context_remove_class(context, class->data);                                                  \
            }                                                                                                          \
        }                                                                                                              \
        g_list_free(classes);                                                                                          \
    } while (0)

#define lupus_error(...) lupus_message(GTK_MESSAGE_ERROR, "<b>Error</b>", __VA_ARGS__)

#define lupus_success(...) lupus_message(GTK_MESSAGE_INFO, "<b>Success</b>", __VA_ARGS__)

#define lupus_message(t, s, ...)                                                                                       \
    do {                                                                                                               \
        GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_USE_HEADER_BAR, t, GTK_BUTTONS_CLOSE, NULL);            \
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(d), s);                                                       \
        gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(d), __VA_ARGS__);                                \
        gtk_dialog_run(GTK_DIALOG(d));                                                                                 \
        gtk_widget_destroy(GTK_WIDGET(d));                                                                             \
    } while (0)

extern char *LUPUS_TOX_DIR;

#endif
