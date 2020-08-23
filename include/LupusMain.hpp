#ifndef __LUPUS_MAIN__
#define __LUPUS_MAIN__

#include "gtkmm/headerbar.h"
#include "gtkmm/object.h"
#include "include/LupusProfile.hpp"
#include "toxpp/Toxpp.hpp"
#include <gtkmm/applicationwindow.h>

namespace Lupus
{
class Main;
}

class Lupus::Main final : public Gtk::ApplicationWindow
{
public:
    Main(void) = delete;
    Main(std::shared_ptr<Toxpp::Self> &toxppSelf) : toxppSelf{toxppSelf}
    {
        auto *headerBar{Gtk::make_managed<Gtk::HeaderBar>()};
        headerBar->pack_start(*Gtk::make_managed<Lupus::Profile>(toxppSelf));
        set_titlebar(*headerBar);

        show_all();
    }
    ~Main(void)
    {
        toxppSelf->save(); // TODO: save here ?
    }

private:
    std::shared_ptr<Toxpp::Self> toxppSelf;
};

#endif
