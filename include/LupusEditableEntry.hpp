#ifndef __LUPUS_EDITABLE_ENTRY__
#define __LUPUS_EDITABLE_ENTRY__

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/enums.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/object.h>
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
    EditableEntry(std::string const &text, unsigned int maxLength = 255, bool bold = false)
        : popover{new Gtk::Popover}
    {
        auto *label{Gtk::make_managed<Gtk::Label>()};
        bold ? label->set_markup("<b>" + text + "</b>") : label->set_text(text);
        label->set_tooltip_text(text);
        label->set_ellipsize(Pango::ELLIPSIZE_END);
        label->set_max_width_chars(0);

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
                goto end;
            }

            if (_signalSubmit.emit(entryText)) {
                bold ? label->set_markup("<b>" + entryText + "</b>") : label->set_text(entryText);
            }

        end:
            popover->popdown();
        });

        add(*label);
        show_all();
    }

    ~EditableEntry(void) { delete popover; }

    decltype(_signalSubmit) signalSubmit(void) { return _signalSubmit; }
};

#endif
