#pragma once

#include <gtkmm/entry.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/popover.h>

namespace Lupus
{
class EditableEntry;
}

class Lupus::EditableEntry final : public Gtk::EventBox
{
private:
    Gtk::Popover *popover;
    sigc::signal<bool, std::string> _signalSubmit;

public:
    EditableEntry(void) = delete;
    EditableEntry(std::string const &text, unsigned int maxLength = 255, bool bold = false);
    ~EditableEntry(void);

    decltype(_signalSubmit) signalSubmit(void);
};
