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
#include <memory>

namespace Lupus
{
class EditableEntry;
}

class Lupus::EditableEntry final : public Gtk::EventBox
{
public:
    EditableEntry(void) = delete;
    EditableEntry(std::string const &text, unsigned int maxLength = 255, bool bold = false)
        : bold{bold}, label{Gtk::make_managed<Gtk::Label>()}, popover{
                                                                  std::make_unique<Gtk::Popover>()}
    {
        this->text(text);
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
        signal_button_release_event().connect([=]([[maybe_unused]] GdkEventButton *event) {
            popover->popup();
            return false;
        });

        submit->signal_clicked().connect([=] {
            auto const &entryText{entry->get_text()};
            if (label->get_text() != entryText) {
                _signalSubmit.emit(entryText);
            }
            popover->popdown();
        });

        add(*label);
        show_all();
    }

    void text(std::string const &text) const
    {
        bold ? label->set_markup("<b>" + text + "</b>") : label->set_text(text);
        label->set_tooltip_text(text);
        return;
    }

    auto signalSubmit(void) { return _signalSubmit; }

private:
    bool bold;
    Gtk::Label *label;
    std::unique_ptr<Gtk::Popover> popover;
    sigc::signal<void, std::string> _signalSubmit;
};

#endif
