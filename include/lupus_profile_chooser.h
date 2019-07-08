#ifndef LUPUS_LUPUS_PROFILE_CHOOSER_H
#define LUPUS_LUPUS_PROFILE_CHOOSER_H

#include <gtk/gtk.h>
#include "lupus_application.h"
#include "lupus_profile_chooser_password_dialog.h"

#define LUPUS_TYPE_PROFILE_CHOOSER lupus_profile_chooser_get_type()

G_DECLARE_FINAL_TYPE(LupusProfileChooser, lupus_profile_chooser, LUPUS, PROFILE_CHOOSER, GtkApplicationWindow)

LupusProfileChooser *lupus_profile_chooser_new(LupusApplication *application);

static void stack_change_callback(GObject *gobject, GParamSpec *pspec, gpointer user_data);

static void register_callback(GtkButton *button, gpointer user_data);

static void login_callback(GtkButton *button, gpointer user_data);

static void set_password_callback(LupusProfileChooserPasswordDialog *password_dialog, gchar *data, gchar **password);

void list_tox_profile(GtkBox *login_box);

#endif