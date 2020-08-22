#include "include/LupusEditableEntry.hpp"
#include "gdkmm/rectangle.h"
#include "gtkmm/box.h"
#include "gtkmm/button.h"
#include "gtkmm/enums.h"
#include "gtkmm/object.h"
#include "gtkmm/popover.h"
#include <gtkmm/entry.h>

using Lupus::EditableEntry;

EditableEntry::EditableEntry(std::string const &text, unsigned int maxLength, bool bold)
    : popover{new Gtk::Popover}
{
    auto *label{Gtk::make_managed<Gtk::Label>(text)};
    if (bold) {
        label->set_markup("<b>" + text + "</b>");
    }

    auto *entry{Gtk::make_managed<Gtk::Entry>()};
    entry->set_text(text);
    entry->set_max_length(maxLength);
    auto *submit{Gtk::make_managed<Gtk::Button>()};
    submit->set_image_from_icon_name("gtk-apply", Gtk::ICON_SIZE_BUTTON);
    submit->set_relief(Gtk::RELIEF_NONE);
    submit->set_focus_on_click(false);
    auto *box{Gtk::make_managed<Gtk::Box>()};
    box->pack_start(*entry, true, true);
    box->pack_start(*submit, false, true);
    box->show_all();

    popover->set_relative_to(*label);
    popover->add(*box);
    signal_button_release_event().connect([=]([[maybe_unused]] GdkEventButton *event) -> bool {
        popover->popup();
        return false;
    });

    submit->signal_clicked().connect([=] {
        auto const &labelText{label->get_text()};
        auto const &entryText{entry->get_text()};

        if (labelText == entryText) {
            return;
        }

        if (_signalSubmit.emit(entryText)) {
            if (bold) {
                label->set_markup("<b>" + entryText + "</b>");
            } else {
                label->set_text(entryText);
            }
        } else {
            entry->set_text(labelText);
        }
    });

    add(*label);
    show_all();
}

EditableEntry::~EditableEntry(void) { delete popover; }

auto EditableEntry::signalSubmit(void) -> decltype(_signalSubmit) { return _signalSubmit; }
