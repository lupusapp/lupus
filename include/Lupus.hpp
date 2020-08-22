#pragma once

#include <glibmm/miscutils.h>
#include <gtkmm/window.h>

namespace Lupus
{
static auto toxConfigDir{Glib::get_user_config_dir() + "/tox/"};
constexpr auto profileBoxWidth = 250;

void messageBox(Gtk::Window &parent, std::string const &message);
} // namespace Lupus

