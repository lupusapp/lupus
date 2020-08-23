#ifndef __LUPUS_PROFILE_CHOOSER__
#define __LUPUS_PROFILE_CHOOSER__

#include "include/Lupus.hpp"
#include "include/LupusMain.hpp"
#include "toxpp/Toxpp.hpp"
#include <exception>
#include <filesystem>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/stack.h>
#include <gtkmm/stackswitcher.h>

/*
 * TODO: contextual menu
 *      - unencrypt profile
 *      - encrypt profile
 *      - delete profile
 */

namespace Lupus
{
class ProfileChooser;
}

class Lupus::ProfileChooser final : public Gtk::ApplicationWindow
{
public:
    ProfileChooser(void)
    {
        auto *loginBox{Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL)};

        auto *regisName{Gtk::make_managed<Gtk::Entry>()};
        regisName->set_placeholder_text("Profile name");
        auto *regisPass{Gtk::make_managed<Gtk::Entry>()};
        regisPass->set_placeholder_text("Profile password");
        auto *regisSubm{Gtk::make_managed<Gtk::Button>("Create profile")};
        auto *regisBox{Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 5)};
        regisBox->property_margin().set_value(20);
        regisBox->pack_start(*regisName, false, true);
        regisBox->pack_start(*regisPass, false, true);
        regisBox->pack_start(*regisSubm, false, true);

        auto *stack{Gtk::make_managed<Gtk::Stack>()};
        stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_CROSSFADE);
        stack->set_transition_duration(200);
        stack->set_vhomogeneous(false);
        stack->set_interpolate_size(true);
        stack->add(*loginBox, "Login", "Login");
        stack->add(*regisBox, "Register", "Register");
        add(*stack);

        auto *stackSwitcher{Gtk::make_managed<Gtk::StackSwitcher>()};
        stackSwitcher->set_stack(*stack);

        auto *headerBar{Gtk::make_managed<Gtk::HeaderBar>()};
        headerBar->set_title("Lupus Profile Chooser");
        headerBar->set_show_close_button(true);
        headerBar->pack_start(*stackSwitcher);
        set_titlebar(*headerBar);

        regisSubm->signal_clicked().connect(
            [=]() { create(regisName->get_text(), regisPass->get_text()); });
        stack->property_visible_child_name().signal_changed().connect([=]() {
            if (stack->property_visible_child_name().get_value() == "Login") {
                listProfiles(*loginBox);
            }
        });

        set_default_size(400, -1);
        set_resizable(false);
        show_all();
    }

private:
    void listProfiles(Gtk::Box &loginBox)
    {
        for (auto &widget : loginBox.get_children()) {
            loginBox.remove(*widget);
            delete widget;
        }

        for (auto &file : std::filesystem::directory_iterator{toxConfigDir}) {
            auto filePath = file.path();
            if (filePath.extension() != ".tox") {
                continue;
            }

            auto *button{Gtk::make_managed<Gtk::Button>(filePath.stem().string())};
            button->set_focus_on_click(false);
            button->set_size_request(0, 50);
            button->set_relief(Gtk::RELIEF_NONE);
            button->signal_clicked().connect(
                sigc::bind(sigc::mem_fun(*this, &ProfileChooser::login), filePath.string()));
            loginBox.pack_start(*button, false, true);
        }

        loginBox.show_all();
    }

    void login(std::string const &profileFilename)
    {
        std::optional<std::string> password{std::nullopt};

        bool isEncrypted;
        try {
            isEncrypted = Toxpp::Self::isEncrypted(profileFilename);
        } catch (std::exception &e) {
            messageBox(*this, e.what());
        }

        if (isEncrypted) {
            auto dialog{std::make_unique<Gtk::Dialog>(Gtk::Dialog{
                "Enter your password", *this, Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR})};
            dialog->add_button("Decrypt", GTK_RESPONSE_ACCEPT);
            dialog->set_size_request(250, 0);
            dialog->set_resizable(false);

            auto entry{Gtk::make_managed<Gtk::Entry>()};
            entry->set_visibility(false);
            entry->set_placeholder_text("Password");

            auto box{static_cast<Gtk::Box *>(dialog->get_child())};
            box->property_margin().set_value(20);
            box->add(*entry);

            dialog->show_all();
            if (dialog->run() != GTK_RESPONSE_ACCEPT || entry->get_text().empty()) {
                return;
            }
            password = entry->get_text();
            dialog->hide();
        }

        try {
            auto *main{new Lupus::Main{new Toxpp::Self{profileFilename, password}}};
            main->signal_hide().connect([=] { delete main; });

            auto application{this->get_application()};
            application->remove_window(*this);
            application->add_window(*main);
            main->present();

            this->~ProfileChooser();
        } catch (std::exception &e) {
            messageBox(*this, e.what());
        }
    }

    void create(std::string const &name, std::string const &password)
    {
        if (name.empty()) {
            messageBox(*this, "Profile name must not be empty.");
            return;
        }

        auto filename{toxConfigDir + name + ".tox"};

        if (std::filesystem::exists(filename)) {
            messageBox(*this, "Profile already exists.");
            return;
        }

        try {
            Toxpp::Self self;
            self.filename(filename);
            self.password(const_cast<std::string &>(password));
            self.save();
        } catch (std::exception &e) {
            messageBox(*this, e.what());
            return;
        }

        messageBox(*this, "Profile created successfully.");
    }
};

#endif
