#ifndef LUPUS_PROFILE_CHOOSER_PASSWORD_DIALOG_H
#define LUPUS_PROFILE_CHOOSER_PASSWORD_DIALOG_H

#include <gtk/gtk.h>

#define LUPUS_TYPE_PROFILE_CHOOSER_PASSWORD_DIALOG lupus_profile_chooser_password_dialog_get_type()

G_DECLARE_FINAL_TYPE(LupusProfileChooserPasswordDialog, lupus_profile_chooser_password_dialog, LUPUS,
                     PROFILE_CHOOSER_PASSWORD_DIALOG, GtkDialog)

LupusProfileChooserPasswordDialog *lupus_profile_chooser_password_dialog_new(void);

static void decrypt_callback(GtkButton *button, gpointer user_data);

#endif