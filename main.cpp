#include "include/LupusProfileChooser.hpp"
#include <memory>

int main(int argc, char **argv)
{
    auto app{Gtk::Application::create("ru.ogromny.lupus")};
    auto profileChooser{std::make_unique<Lupus::ProfileChooser>()};

    app->signal_activate().connect([&]() {
        app->add_window(*profileChooser);
        profileChooser->present();
    });

    return app->run(argc, argv);
}

