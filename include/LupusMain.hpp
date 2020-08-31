#ifndef __LUPUS_MAIN__
#define __LUPUS_MAIN__

#include "gtkmm/separator.h"
#include "include/LupusFriendsBox.hpp"
#include "include/LupusProfile.hpp"
#include "toxpp/Toxpp.hpp"
#include <gtkmm/applicationwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/enums.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/object.h>

namespace Lupus
{
class Main;
}

class Lupus::Main final : public Gtk::ApplicationWindow
{
public:
    Main(void) = delete;
    Main(std::shared_ptr<Toxpp::Self> &toxppSelf)
        : toxppSelf{toxppSelf}, friendsBox{Gtk::make_managed<Lupus::FriendsBox>(toxppSelf)}
    {
        auto *headerBar{Gtk::make_managed<Gtk::HeaderBar>()};
        headerBar->pack_start(*Gtk::make_managed<Lupus::Profile>(toxppSelf));
        headerBar->set_show_close_button();
        set_titlebar(*headerBar);

        auto *box{Gtk::make_managed<Gtk::Box>()};
        box->pack_start(*friendsBox, false, true);
        box->pack_start(*Gtk::make_managed<Gtk::Separator>(Gtk::ORIENTATION_VERTICAL), false, true);

        add(*box);

        show_all();
    }
    ~Main(void)
    {
        toxppSelf->save(); // TODO: save here ?
    }

private:
    std::shared_ptr<Toxpp::Self> toxppSelf;
    Lupus::FriendsBox *friendsBox;
};

#endif
