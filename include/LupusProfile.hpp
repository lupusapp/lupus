#pragma once

#include "gtkmm/box.h"
#include "gtkmm/enums.h"
#include "gtkmm/eventbox.h"
#include "gtkmm/object.h"
#include "gtkmm/window.h"
#include "include/Lupus.hpp"
#include "include/LupusEditableEntry.hpp"
#include "toxpp/Self.hpp"
#include <exception>

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

        name->signalSubmit().connect([=](std::string name) -> bool {
            try {
                toxppSelf->name(name);
                return true;
            } catch (std::exception &e) {
                messageBox(dynamic_cast<Gtk::Window *>(get_parent()), std::string{e.what()});
                return false;
            }
        });

        statusMessage->signalSubmit().connect([=](std::string statusMessage) -> bool {
            try {
                toxppSelf->statusMessage(statusMessage);
                return true;
            } catch (std::exception &e) {
                messageBox(dynamic_cast<Gtk::Window *>(get_parent()), std::string{e.what()});
                return false;
            }
        });

        show_all();
    }

private:
    Toxpp::Self *toxppSelf;
    Lupus::EditableEntry *name;
    Lupus::EditableEntry *statusMessage;
};
