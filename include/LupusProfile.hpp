#pragma once

#include "include/Lupus.hpp"
#include "include/LupusEditableEntry.hpp"
#include "toxpp/Self.hpp"
#include <exception>
#include <gdkmm/pixbuf.h>
#include <gdkmm/pixbufloader.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/enums.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/object.h>
#include <gtkmm/separator.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/window.h>
#include <memory>
#include <utility>

#define STANDARD_SIZE 36

// TODO: change avatar when clicking on avatar

namespace Lupus
{
class Profile;
}

class Lupus::Profile final : public Gtk::EventBox
{
public:
    Profile(void) = delete;
    Profile(std::shared_ptr<Toxpp::Self> &toxppSelf)
        : toxppSelf{toxppSelf}, avatar{Gtk::make_managed<Gtk::Image>()},
          popover{std::make_unique<Gtk::Menu>()}
    {
        auto *name{Gtk::make_managed<Lupus::EditableEntry>(toxppSelf->name(),
                                                           toxppSelf->nameMaxSize, true)};
        auto *statusMessage{Gtk::make_managed<Lupus::EditableEntry>(
            toxppSelf->statusMessage(), toxppSelf->statusMessageMaxSize)};
        auto *lbox{Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL)};
        lbox->pack_start(*name, true, true);
        lbox->pack_start(*statusMessage, true, true);

        avatar->get_style_context()->add_class("profile");
        refreshAvatar();
        auto *rbox{Gtk::make_managed<Gtk::EventBox>()};
        rbox->add(*avatar);

        auto *box{Gtk::make_managed<Gtk::Box>()};
        box->pack_start(*lbox, true, true);
        box->pack_start(*rbox, false, true);
        box->property_margin().set_value(5);

        add(*box);
        constructPopover();

        name->signalSubmit().connect([=](std::string name) {
            try {
                toxppSelf->name(name);
            } catch (std::exception &e) {
                messageBox(dynamic_cast<Gtk::Window *>(get_parent()), {e.what()});
            }
        });

        statusMessage->signalSubmit().connect([=](std::string statusMessage) {
            try {
                toxppSelf->statusMessage(statusMessage);
            } catch (std::exception &e) {
                messageBox(dynamic_cast<Gtk::Window *>(get_parent()), {e.what()});
            }
        });

        rbox->signal_button_press_event().connect([=](GdkEventButton *event) {
            if (event->type != GDK_BUTTON_PRESS) {
                return false;
            }

            if (event->button == 3) {
                popover->popup_at_pointer(nullptr);
            }

            return false;
        });

        auto updateClasses{
            [=](Toxpp::Self::ConnectionStatus connectionStatus, Toxpp::Self::Status status) {
                for (auto const &name : avatar->get_style_context()->list_classes()) {
                    if (name.find("profile--") != std::string::npos) {
                        avatar->get_style_context()->remove_class(name);
                    }
                }

                if (connectionStatus != Toxpp::Self::ConnectionStatus::NONE) {
                    switch (status) {
                    case Toxpp::Self::Status::NONE:
                        avatar->get_style_context()->add_class("profile--none");
                        break;
                    case Toxpp::Self::Status::AWAY:
                        avatar->get_style_context()->add_class("profile--away");
                        break;
                    case Toxpp::Self::Status::BUSY:
                        avatar->get_style_context()->add_class("profile--busy");
                    }
                }
            }};

        toxppSelf->nameSignal().connect([=](auto text) { name->text(text); });
        toxppSelf->statusMessageSignal().connect([=](auto text) { statusMessage->text(text); });
        toxppSelf->connectionStatusSignal().connect(
            [=](auto connectionStatus) { updateClasses(connectionStatus, toxppSelf->status()); });
        toxppSelf->statusSignal().connect(
            [=](auto status) { updateClasses(toxppSelf->connectionStatus(), status); });

        set_size_request(profileBoxWidth);
        show_all();
    }

private:
    void constructPopover(void)
    {
        auto createMenuItem{[=](std::string name, std::string resource) {
            auto *label{Gtk::make_managed<Gtk::Label>(name)};
            auto *image{Gtk::make_managed<Gtk::Image>()};
            image->set_from_resource(resource);
            auto *box{Gtk::make_managed<Gtk::Box>()};
            box->pack_start(*image, false, true);
            box->pack_start(*label, true, true);
            return Gtk::make_managed<Gtk::MenuItem>(*box);
        }};

        auto *none{createMenuItem("Online", "/ru/ogromny/lupus/status_none.svg")};
        auto *away{createMenuItem("Away", "/ru/ogromny/lupus/status_away.svg")};
        auto *busy{createMenuItem("Busy", "/ru/ogromny/lupus/status_busy.svg")};
        auto *myID{createMenuItem("My ToxID", "/ru/ogromny/lupus/biometric.svg")};
        auto *about{createMenuItem("About", "/ru/ogromny/lupus/lupus_tiny.svg")};

        none->signal_activate().connect([=] { toxppSelf->status(Toxpp::Self::Status::NONE); });
        away->signal_activate().connect([=] { toxppSelf->status(Toxpp::Self::Status::AWAY); });
        busy->signal_activate().connect([=] { toxppSelf->status(Toxpp::Self::Status::BUSY); });
        myID->signal_activate().connect([=] {
            auto dialog{std::make_unique<Gtk::Dialog>("My ToxID", Gtk::DIALOG_USE_HEADER_BAR)};
            dialog->set_resizable(false);
            dialog->set_border_width(5);

            auto *label{Gtk::make_managed<Gtk::Label>(toxppSelf->addressHex())};
            label->set_selectable();

            dynamic_cast<Gtk::Box *>(dialog->get_child())->add(*label);

            dialog->show_all();
            dialog->run();
            dialog->hide();
        });
        about->signal_activate().connect([=] {
            auto dialog{std::make_unique<Gtk::AboutDialog>(true)};
            dialog->set_program_name("Lupus");
            dialog->set_version(Lupus::version);
            dialog->set_copyright("© 2019-2020 Ogromny");
            dialog->set_wrap_license(true);
            dialog->set_license_type(Gtk::LICENSE_GPL_3_0);
            dialog->set_website("https://github.com/LupusApp/Lupus");
            dialog->set_website_label("GitHub");
            dialog->set_authors({"Ogromny"});
            dialog->set_logo(Gdk::Pixbuf::create_from_resource("/ru/ogromny/lupus/lupus.svg"));
            dialog->run();
            dialog->hide();
        });

        popover->append(*none);
        popover->append(*away);
        popover->append(*busy);
        popover->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());
        popover->append(*myID);
        popover->append(*Gtk::make_managed<Gtk::SeparatorMenuItem>());
        popover->append(*about);

        popover->show_all();
    }

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
    Gtk::Image *avatar;
    std::unique_ptr<Gtk::Menu> popover;
};
