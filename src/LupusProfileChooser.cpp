#include "include/LupusProfileChooser.hpp"
#include "glibmm/refptr.h"
#include "include/Lupus.hpp"
#include "include/LupusMain.hpp"
#include "include/ToxSelf.hpp"
#include "include/lupus_main.h"
#include "sigc++/adaptors/bind.h"
#include "sigc++/functors/mem_fun.h"
#include "sigc++/functors/ptr_fun.h"
#include <filesystem>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/stack.h>
#include <gtkmm/stackswitcher.h>
#include <iostream>
#include <memory>

/*
 * TODO: contextual menu
 *      - unencrypt profile
 *      - encrypt profile
 *      - delete profile
 */

using Lupus::ProfileChooser;

ProfileChooser::ProfileChooser(void) : Gtk::ApplicationWindow()
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

void ProfileChooser::listProfiles(Gtk::Box &loginBox)
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

void ProfileChooser::login(std::string const &profileFilename)
{
    std::optional<std::string> password{std::nullopt};

    if (ToxSelf::isEncrypted(profileFilename)) {
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
        auto application{this->get_application()};
        application->remove_window(*this);

        auto *main{new Lupus::Main{new ToxSelf{profileFilename, password}}};
        main->signal_hide().connect([=] { delete main; });
        application->add_window(*main);
        main->present();

        this->~ProfileChooser();
    } catch (std::runtime_error &e) {
        messageBox(*this, e.what());
    }
}

void ProfileChooser::create(std::string const &name, std::string const &password)
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
        ToxSelf self;
        self.filename(filename);
        self.password(const_cast<std::string &>(password));
        self.save();
    } catch (std::runtime_error &e) {
        messageBox(*this, e.what());
        return;
    }

    messageBox(*this, "Profile created successfully.");
}
