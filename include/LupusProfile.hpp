#pragma once

#include "gdkmm/pixbuf.h"
#include "gdkmm/pixbufloader.h"
#include "gtkmm/box.h"
#include "gtkmm/enums.h"
#include "gtkmm/eventbox.h"
#include "gtkmm/image.h"
#include "gtkmm/object.h"
#include "gtkmm/window.h"
#include "include/Lupus.hpp"
#include "include/LupusEditableEntry.hpp"
#include "toxpp/Self.hpp"
#include <exception>
#include <memory>

#define STANDARD_SIZE 36

namespace Lupus
{
class Profile;
}

class Lupus::Profile final : public Gtk::EventBox
{
public:
    Profile(void) = delete;
    Profile(std::shared_ptr<Toxpp::Self> &toxppSelf)
        : toxppSelf{toxppSelf}, name{Gtk::make_managed<Lupus::EditableEntry>(
                                    toxppSelf->name(), toxppSelf->nameMaxSize, true)},
          statusMessage{Gtk::make_managed<Lupus::EditableEntry>(toxppSelf->statusMessage(),
                                                                toxppSelf->statusMessageMaxSize)},
          avatar{Gtk::make_managed<Gtk::Image>()}
    {
        auto *lbox{Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL)};
        lbox->pack_start(*name, true, true);
        lbox->pack_start(*statusMessage, true, true);

        auto *box{Gtk::make_managed<Gtk::Box>()};
        box->pack_start(*lbox, true, true);
        box->pack_start(*avatar, false, true);
        box->property_margin().set_value(5);

        avatar->get_style_context()->add_class("profile");
        refreshAvatar();

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
    void refreshAvatar(void)
    {
        auto _avatar{toxppSelf->avatar().avatar()};

        auto loader{Gdk::PixbufLoader::create("png")};
        loader->write(_avatar.data(), _avatar.size());
        auto pixbuf{loader->get_pixbuf()};
        loader->close();

        avatar->set(pixbuf->scale_simple(STANDARD_SIZE, STANDARD_SIZE, Gdk::INTERP_BILINEAR));
    }

private:
    std::shared_ptr<Toxpp::Self> toxppSelf;
    Lupus::EditableEntry *name;
    Lupus::EditableEntry *statusMessage;
    Gtk::Image *avatar;
};
