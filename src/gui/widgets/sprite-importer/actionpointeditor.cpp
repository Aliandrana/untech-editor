#include "actionpointeditor.h"
#include "gui/undo/actionhelper.h"

using namespace UnTech::Widgets::SpriteImporter;
namespace SI = UnTech::SpriteImporter;

SIMPLE_UNDO_ACTION(actionPoint_setLocation,
                   SI::ActionPoint, UnTech::upoint, location, setLocation,
                   Signals::actionPointChanged,
                   "Move Action Point")

SIMPLE_UNDO_ACTION(actionPoint_setParameter,
                   SI::ActionPoint, unsigned, parameter, setParameter,
                   Signals::actionPointChanged,
                   "Change Action Point Parameter")

ActionPointEditor::ActionPointEditor(Undo::UndoStack& undoStack)
    : widget()
    , _undoStack(undoStack)
    , _actionPoint()
    , _locationSpinButtons()
    , _parameterEntry()
    , _locationLabel(_("Location:"), Gtk::ALIGN_START)
    , _locationCommaLabel(" ,  ")
    , _parameterLabel(_("Parameter:"), Gtk::ALIGN_START)
    , _updatingValues(false)
{
    widget.set_row_spacing(DEFAULT_ROW_SPACING);

    widget.attach(_locationLabel, 0, 0, 1, 1);
    widget.attach(_locationSpinButtons.xSpin, 1, 0, 1, 1);
    widget.attach(_locationCommaLabel, 2, 0, 1, 1);
    widget.attach(_locationSpinButtons.ySpin, 3, 0, 1, 1);

    widget.attach(_parameterLabel, 0, 1, 1, 1);
    widget.attach(_parameterEntry, 1, 1, 3, 1);

    updateGuiValues();

    /*
     * SLOTS
     * =====
     */

    /** Set location signal */
    _locationSpinButtons.signal_valueChanged.connect([this](void) {
        if (_actionPoint && !_updatingValues) {
            actionPoint_setLocation(_undoStack, _actionPoint, _locationSpinButtons.value());
        }
    });

    /* Set Parameter has finished editing */
    // signal_editing_done does not work
    // using activate and focus out instead.
    _parameterEntry.signal_activate().connect(sigc::mem_fun(
        *this, &ActionPointEditor::onParameterFinishedEditing));
    _parameterEntry.signal_focus_out_event().connect([this](GdkEventFocus*) {
        this->onParameterFinishedEditing();
        return false;
    });

    /* Update gui if object has changed */
    Signals::actionPointChanged.connect([this](const std::shared_ptr<SI::ActionPoint> obj) {
        if (_actionPoint == obj) {
            updateGuiValues();
        }
    });

    /* Update location range if necessary */
    Signals::frameSizeChanged.connect([this](const std::shared_ptr<SI::Frame> frame) {
        if (_actionPoint) {
            const auto f = _actionPoint->frame();
            if (frame == f) {
                _locationSpinButtons.set_range(frame->locationSize());
            }
        }
    });

    Signals::frameSetGridChanged.connect([this](const std::shared_ptr<SI::FrameSet> fs) {
        if (_actionPoint) {
            const auto frame = _actionPoint->frame();
            if (frame->frameSet() == fs) {
                _locationSpinButtons.set_range(frame->locationSize());
            }
        }
    });
}

void ActionPointEditor::updateGuiValues()
{
    if (_actionPoint) {
        auto frame = _actionPoint->frame();

        _updatingValues = true;

        if (frame) {
            _locationSpinButtons.set_range(frame->locationSize());
        }
        _locationSpinButtons.set_value(_actionPoint->location());
        _parameterEntry.set_text(Glib::ustring::compose("%1", _actionPoint->parameter()));

        _updatingValues = false;

        widget.set_sensitive(true);
    }
    else {
        static const upoint zeroPoint = { 0, 0 };

        _locationSpinButtons.set_value(zeroPoint);
        _parameterEntry.set_text("");

        widget.set_sensitive(false);
    }
}

void ActionPointEditor::onParameterFinishedEditing()
{
    if (_actionPoint && !_updatingValues) {
        auto value = UnTech::String::toUint8(_parameterEntry.get_text());
        if (value.second) {
            actionPoint_setParameter(_undoStack, _actionPoint, value.first);
        }
        else {
            updateGuiValues();
        }
    }
}
