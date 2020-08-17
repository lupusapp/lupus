#include "include/Lupus.hpp"
#include <gtkmm/messagedialog.h>

void Lupus::messageBox(Gtk::Window &parent, std::string const &message)
{
    Gtk::MessageDialog{parent, message, true, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true}.run();
}
