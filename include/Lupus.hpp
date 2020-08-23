#ifndef __LUPUS_LUPUS__
#define __LUPUS_LUPUS__

#include "glibmm/refptr.h"
#include "gtkmm/enums.h"
#include "gtkmm/messagedialog.h"
#include <glibmm/miscutils.h>
#include <gtkmm/window.h>

namespace Lupus
{
static auto toxConfigDir{Glib::get_user_config_dir() + "/tox/"};
constexpr auto profileBoxWidth{250};

void messageBox(Gtk::Window *parent, const std::string &message)
{
    Gtk::MessageDialog{*parent, message, true, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true}.run();
}
void messageBox(Gtk::Window &parent, std::string const &message) { messageBox(&parent, message); }
} // namespace Lupus

#endif
