#ifndef __LUPUS_FRIENDS_BOX__
#define __LUPUS_FRIENDS_BOX__

#include "include/Lupus.hpp"
#include "toxpp/Self.hpp"
#include <cstdint>
#include <exception>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/object.h>
#include <gtkmm/window.h>
#include <memory>
#include <sodium/utils.h>

namespace Lupus
{
class FriendsBox;
}

class Lupus::FriendsBox final : public Gtk::EventBox
{
public:
    FriendsBox(void) = delete;
    FriendsBox(std::shared_ptr<Toxpp::Self> &toxppSelf)
        : toxppSelf{toxppSelf}, popover{std::make_unique<Gtk::Menu>()}
    {
        set_size_request(Lupus::friendsBoxWidth);
        set_halign(Gtk::ALIGN_START);

        constructPopover();

        show_all();

        signal_button_press_event().connect([this](GdkEventButton *event) {
            if (event->type != GDK_BUTTON_PRESS) {
                return false;
            }

            if (event->button == 3) {
                popover->popup_at_pointer(nullptr);
            }

            return false;
        });
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

        auto *plus{createMenuItem("Add friend", "/ru/ogromny/lupus/plus_tiny.svg")};
        plus->signal_activate().connect([=] {
            // TODO: persistent dialog if failure
            auto dialog{std::make_unique<Gtk::Dialog>("Add friend", Gtk::DIALOG_USE_HEADER_BAR)};
            dialog->set_resizable(false);
            dialog->set_border_width(5);
            dialog->add_button("Add", Gtk::RESPONSE_YES);

            auto *addressHex{Gtk::make_managed<Gtk::Entry>()};
            addressHex->set_max_length(Toxpp::Self::addressHexSize);
            addressHex->set_placeholder_text("Friend's address");

            auto *message{Gtk::make_managed<Gtk::Entry>()};
            message->set_max_length(Toxpp::Self::messageMaxSize);
            message->set_placeholder_text("Message");

            auto *box{dynamic_cast<Gtk::Box *>(dialog->get_child())};
            box->set_spacing(5);
            box->pack_start(*addressHex);
            box->pack_start(*message);
            box->show_all();

            if (dialog->run() == Gtk::RESPONSE_YES) {
                std::vector<std::uint8_t> addressBin(Toxpp::Self::addressBinSize);
                std::string addressHexString{addressHex->get_text()};
                std::cout << addressHexString.size();
                sodium_hex2bin(addressBin.data(), addressBin.size(), addressHexString.data(),
                               addressHexString.size(), nullptr, nullptr, nullptr);
                try {
                    toxppSelf->addFriend(addressBin, message->get_text());
                } catch (std::exception &e) {
                    // TODO: change this
                    messageBox(dynamic_cast<Gtk::Window *>(get_parent()->get_parent()), e.what());
                }
            }
            dialog->hide();
        });

        popover->append(*plus);

        popover->show_all();
    }

private:
    std::shared_ptr<Toxpp::Self> toxppSelf;
    std::unique_ptr<Gtk::Menu> popover;
};

#endif
