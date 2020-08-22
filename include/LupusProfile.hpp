#pragma once

#include "gtkmm/eventbox.h"
#include "include/LupusEditableEntry.hpp"
#include "include/ToxSelf.hpp"

namespace Lupus
{
class Profile;
}

class Lupus::Profile final : public Gtk::EventBox
{
public:
    Profile(void) = delete;
    Profile(ToxSelf *toxSelf);

private:
    ToxSelf *toxSelf;
    Lupus::EditableEntry *name;
    Lupus::EditableEntry *statusMessage;
};
