#include "include/LupusProfileChooser.hpp"
#include <gdkmm/screen.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/stylecontext.h>
#include <memory>

int main(int argc, char **argv)
{
    auto app{Gtk::Application::create("ru.ogromny.lupus")};
    auto profileChooser{std::make_unique<Lupus::ProfileChooser>()};

    auto provider{Gtk::CssProvider::create()};
    provider->load_from_resource("/ru/ogromny/lupus/lupus.css");
    Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), provider,
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    app->signal_activate().connect([&]() {
        app->add_window(*profileChooser);
        profileChooser->present();
    });

    return app->run(argc, argv);
}

