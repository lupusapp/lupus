#pragma once

#include "gtkmm/box.h"
#include "gtkmm/enums.h"
#include "gtkmm/eventbox.h"
#include "gtkmm/object.h"
#include "include/Lupus.hpp"
#include "include/LupusEditableEntry.hpp"
#include "toxpp/Self.hpp"

namespace Lupus
{
class Profile;
}

class Lupus::Profile final : public Gtk::EventBox
{
public:
    Profile(void) = delete;
    Profile(Toxpp::Self *toxppSelf)
        : toxppSelf{toxppSelf}, name{Gtk::make_managed<Lupus::EditableEntry>(
                                    toxppSelf->name(), toxppSelf->nameMaxSize)},
          statusMessage{Gtk::make_managed<Lupus::EditableEntry>(toxppSelf->statusMessage(),
                                                                toxppSelf->statusMessageMaxSize)}
    {
        auto *lbox{Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL)};
        lbox->pack_start(*name, true, true);
        lbox->pack_start(*statusMessage, true, true);

        auto *box{Gtk::make_managed<Gtk::Box>()};
        box->pack_start(*lbox, true, true);

        add(*box);
        set_size_request(Lupus::profileBoxWidth);
        show_all();
    }

private:
    Toxpp::Self *toxppSelf;
    Lupus::EditableEntry *name;
    Lupus::EditableEntry *statusMessage;
};
