#ifndef __LUPUS_UTILS_H__
#define __LUPUS_UTILS_H__

#include <gtk/gtk.h>
#include <tox/tox.h>

#define remove_class_with_prefix(i, n)                                         \
    ({                                                                         \
        GtkStyleContext *context =                                             \
            gtk_widget_get_style_context(GTK_WIDGET(i));                       \
        GList *classes = gtk_style_context_list_classes(context);              \
        for (GList *class = classes; class != NULL; class = class->next) {     \
            if (g_str_has_prefix(class->data, n)) {                            \
                gtk_style_context_remove_class(context, class->data);          \
            }                                                                  \
        }                                                                      \
        g_list_free(classes);                                                  \
    })

#endif