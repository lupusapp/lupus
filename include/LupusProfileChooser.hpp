#pragma once

#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>

namespace Lupus
{
class ProfileChooser;
}

class Lupus::ProfileChooser final : public Gtk::ApplicationWindow
{
public:
    ProfileChooser(void);

private:
    void listProfiles(Gtk::Box &loginBox);
    void login(std::string const &profileFilename);
    void create(std::string const &name, std::string const &password);
};
