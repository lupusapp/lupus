#ifndef __LUPUS_LUPUS_H__
#define __LUPUS_LUPUS_H__

#define LUPUS_APPLICATION_ID "ru.ogromny.lupus"
#define LUPUS_RESOURCES "/ru/ogromny/lupus"

#define lupus_error(i, ...)                                                    \
    (lupus_message(GTK_MESSAGE_ERROR, "<b>Error</b>", i, __VA_ARGS__))

#define lupus_success(i, ...)                                                  \
    (lupus_message(GTK_MESSAGE_INFO, "<b>Success</b>", i, __VA_ARGS__))

#define lupus_message(t, s, i, ...)                                            \
    ({                                                                         \
        GtkWidget *d =                                                         \
            gtk_message_dialog_new(GTK_WINDOW(i), GTK_DIALOG_USE_HEADER_BAR,   \
                                   t, GTK_BUTTONS_CLOSE, NULL);                \
        gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(d), s);               \
        gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(d),      \
                                                   __VA_ARGS__);               \
        gtk_dialog_run(GTK_DIALOG(d));                                         \
        gtk_widget_destroy(GTK_WIDGET(d));                                     \
    })

extern char *LUPUS_TOX_DIR;

#endif