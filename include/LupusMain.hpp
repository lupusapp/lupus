#pragma once

#include "gtkmm/headerbar.h"
#include "gtkmm/object.h"
#include "include/ToxSelf.hpp"
#include <gtkmm/applicationwindow.h>

namespace Lupus
{
class Main;
}

class Lupus::Main final : public Gtk::ApplicationWindow
{
public:
    Main(ToxSelf *toxSelf);
    ~Main(void);

private:
    // TODO: shared_ptr ?
    ToxSelf *toxSelf;
};
