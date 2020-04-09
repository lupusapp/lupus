#ifndef __LUPUS_LUPUS_H__
#define __LUPUS_LUPUS_H__

#define LUPUS_APPLICATION_ID "ru.ogromny.lupus"
#define LUPUS_RESOURCES "/ru/ogromny/lupus"
#define LUPUS_VERSION "0.9.9"

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

#define CONCAT(a, b, c, d) a##b##c##d
#define header_getter(aclass, bclass, name, type) type CONCAT(lupus_, aclass, _get_, name)(Lupus##bclass * instance)
#define header_setter(aclass, bclass, name, type)                                                                      \
    void CONCAT(lupus_, aclass, _set_, name)(Lupus##bclass * instance, type name)
#define header_getter_setter(aclass, bclass, name, type)                                                               \
    header_getter(aclass, bclass, name, type);                                                                         \
    header_setter(aclass, bclass, name, type);

#define getter(aclass, bclass, name, type)                                                                             \
    header_getter(aclass, bclass, name, type) { return instance->name; }
#define setter(aclass, bclass, name, type)                                                                             \
    header_setter(aclass, bclass, name, type) { g_object_set(instance, #name, name, NULL); }
#define getter_setter(aclass, bclass, name, type)                                                                      \
    getter(aclass, bclass, name, type);                                                                                \
    setter(aclass, bclass, name, type);

extern char *LUPUS_TOX_DIR;

#endif