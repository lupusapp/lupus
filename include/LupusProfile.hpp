#pragma once

#include "gdkmm/pixbuf.h"
#include "gdkmm/pixbufloader.h"
#include "gtkmm/box.h"
#include "gtkmm/enums.h"
#include "gtkmm/eventbox.h"
#include "gtkmm/image.h"
#include "gtkmm/label.h"
#include "gtkmm/menu.h"
#include "gtkmm/menuitem.h"
#include "gtkmm/object.h"
#include "gtkmm/separator.h"
#include "gtkmm/window.h"
#include "include/Lupus.hpp"
#include "include/LupusEditableEntry.hpp"
#include "toxpp/Self.hpp"
#include <exception>
#include <memory>
#include <utility>

#define STANDARD_SIZE 36

namespace Lupus
{
class Profile;
}

using Gdk::PixbufLoader;
using Gtk::Box, Gtk::make_managed, Gtk::Window, Gtk::Image, Gtk::Menu, Gtk::Label, Gtk::MenuItem,
    Gtk::Separator;
using Lupus::EditableEntry, Lupus::profileBoxWidth, Toxpp::Self;
using std::string, std::exception, std::shared_ptr, std::unique_ptr, std::tuple;

class Lupus::Profile final : public Gtk::EventBox
{
public:
    Profile(void) = delete;
    Profile(std::shared_ptr<Toxpp::Self> &toxppSelf)
        : toxppSelf{toxppSelf}, avatar{Gtk::make_managed<Gtk::Image>()}, popover{unique_ptr<Menu>()}
    {

        auto *name{make_managed<EditableEntry>(toxppSelf->name(), toxppSelf->nameMaxSize, true)};
        auto *statusMessage{make_managed<EditableEntry>(toxppSelf->statusMessage(),
                                                        toxppSelf->statusMessageMaxSize)};

        auto *lbox{make_managed<Box>(Gtk::ORIENTATION_VERTICAL)};
        lbox->pack_start(*name, true, true);
        lbox->pack_start(*statusMessage, true, true);

        auto *box{make_managed<Box>()};
        box->pack_start(*lbox, true, true);
        box->pack_start(*avatar, false, true);
        box->property_margin().set_value(5);

        avatar->get_style_context()->add_class("profile");
        refreshAvatar();

        add(*box);
        set_size_request(profileBoxWidth);

        name->signalSubmit().connect([=](string name) -> bool {
            try {
                toxppSelf->name(name);
                return true;
            } catch (exception &e) {
                messageBox(dynamic_cast<Window *>(get_parent()), string{e.what()});
                return false;
            }
        });

        statusMessage->signalSubmit().connect([=](string statusMessage) -> bool {
            try {
                toxppSelf->statusMessage(statusMessage);
                return true;
            } catch (exception &e) {
                messageBox(dynamic_cast<Window *>(get_parent()), string{e.what()});
                return false;
            }
        });

        toxppSelf->connectionStatusSignal().connect([=](Self::ConnectionStatus status) {
            if (status != Toxpp::Self::ConnectionStatus::NONE) {
                avatar->get_style_context()->add_class("profile--none");
            } else {
                avatar->get_style_context()->remove_class("profile--none");
            }
        });

        show_all();
    }

private:
    void refreshAvatar(void)
    {
        auto _avatar{toxppSelf->avatar().avatar()};

        auto loader{PixbufLoader::create("png")};
        loader->write(_avatar.data(), _avatar.size());
        auto pixbuf{loader->get_pixbuf()};
        loader->close();

        avatar->set(pixbuf->scale_simple(STANDARD_SIZE, STANDARD_SIZE, Gdk::INTERP_BILINEAR));
    }

private:
    shared_ptr<Self> toxppSelf;
    Image *avatar;
    unique_ptr<Menu> popover;
};
