#include "include/LupusProfile.hpp"
#include "gtkmm/enums.h"
#include "gtkmm/object.h"
#include "include/Lupus.hpp"
#include <gtkmm/box.h>

using Lupus::Profile;

Profile::Profile(ToxSelf *toxSelf) : toxSelf{toxSelf}
{
    name = Gtk::make_managed<Lupus::EditableEntry>(toxSelf->name(), toxSelf->nameMaxSize, true);
    statusMessage = Gtk::make_managed<Lupus::EditableEntry>(toxSelf->statusMessage(),
                                                            toxSelf->statusMessageMaxSize);
    auto *lbox{Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL)};
    lbox->pack_start(*name, true, true);
    lbox->pack_start(*statusMessage, true, true);

    auto *box{Gtk::make_managed<Gtk::Box>()};
    box->pack_start(*lbox, true, true);

    add(*box);
    set_size_request(Lupus::profileBoxWidth);
    show_all();
}
