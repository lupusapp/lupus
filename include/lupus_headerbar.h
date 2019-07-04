//
// Created by ogromny on 13/06/19.
//

#ifndef LUPUS_LUPUS_HEADERBAR_H
#define LUPUS_LUPUS_HEADERBAR_H

#include <gtk/gtk.h>

#define LUPUS_TYPE_HEADERBAR lupus_headerbar_get_type()

G_DECLARE_FINAL_TYPE(LupusHeaderbar, lupus_headerbar, LUPUS, HEADERBAR, GtkHeaderBar)

LupusHeaderbar *lupus_headerbar_new(void);

#endif //LUPUS_LUPUS_HEADERBAR_H
