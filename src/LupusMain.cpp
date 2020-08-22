#include "include/LupusMain.hpp"
#include "gtkmm/box.h"
#include "gtkmm/entry.h"
#include "gtkmm/enums.h"
#include "gtkmm/headerbar.h"
#include "gtkmm/object.h"
#include "include/LupusEditableEntry.hpp"
#include "include/LupusProfile.hpp"
#include <gtkmm/separator.h>

using Lupus::Main;

Main::Main(ToxSelf *toxSelf) : toxSelf{toxSelf}
{
    auto *headerbar{Gtk::make_managed<Gtk::HeaderBar>()};
    headerbar->set_show_close_button();
    headerbar->pack_start(*Gtk::make_managed<Lupus::Profile>(toxSelf));
    headerbar->pack_start(*Gtk::make_managed<Gtk::Separator>(Gtk::ORIENTATION_VERTICAL));

    set_titlebar(*headerbar);
    show_all();
}

Main::~Main(void)
{
    // TODO: save here ?
    toxSelf->save();
    delete toxSelf;
}
