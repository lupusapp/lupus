#ifndef __LUPUS_MAIN__
#define __LUPUS_MAIN__

#include "gtkmm/headerbar.h"
#include "gtkmm/object.h"
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
    Main(Toxpp::Self *toxppSelf) : toxppSelf{toxppSelf} { show_all(); }
    ~Main(void)
    {
        toxppSelf->save(); // TODO: save here ?
        delete toxppSelf;
    }

private:
    // TODO: shared_ptr ?
    Toxpp::Self *toxppSelf;
};

#endif
