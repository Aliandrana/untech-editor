#include "framesetpropertieseditor.h"
#include "gui/undo/actionhelper.h"
#include <iomanip>

using namespace UnTech::Widgets::SpriteImporter;

PARAMETER_UNDO_ACTION2(frameSet_Grid_setFrameSize,
                       SI::FrameSet, grid, UnTech::usize, frameSize, setFrameSize,
                       Signals::frameSetChanged, Signals::frameSetGridChanged,
                       "Change Grid Frame Size")

PARAMETER_UNDO_ACTION2(frameSet_Grid_setOffset,
                       SI::FrameSet, grid, UnTech::upoint, offset, setOffset,
                       Signals::frameSetChanged, Signals::frameSetGridChanged,
                       "Change Grid Offset")

PARAMETER_UNDO_ACTION2(frameSet_Grid_setPadding,
                       SI::FrameSet, grid, UnTech::usize, padding, setPadding,
                       Signals::frameSetChanged, Signals::frameSetGridChanged,
                       "Change Grid Padding")

PARAMETER_UNDO_ACTION2(frameSet_Grid_setOrigin,
                       SI::FrameSet, grid, UnTech::upoint, origin, setOrigin,
                       Signals::frameSetChanged, Signals::frameSetGridChanged,
                       "Change Grid Origin")

FrameSetPropertiesEditor::FrameSetPropertiesEditor()
    : widget()
    , _frameSet(nullptr)
    , _imageFilenameBox(Gtk::ORIENTATION_HORIZONTAL)
    , _imageFilenameEntry()
    , _imageFilenameButton(_("..."))
    , _transparentColorBox(Gtk::ORIENTATION_HORIZONTAL)
    , _transparentColorEntry()
    , _transparentColorButton("    ")
    , _gridFrameSizeSpinButtons()
    , _gridOffsetSpinButtons()
    , _gridPaddingSpinButtons()
    , _gridOriginSpinButtons()
    , _imageFilenameLabel(_("Image:"), Gtk::ALIGN_START)
    , _transparentColorLabel(_("Transparent:"), Gtk::ALIGN_START)
    , _gridFrameSizeLabel(_("Grid Size:"), Gtk::ALIGN_START)
    , _gridFrameSizeCrossLabel(_(" x "))
    , _gridOffsetLabel(_("Grid Offset:"), Gtk::ALIGN_START)
    , _gridOffsetCommaLabel(_(" , "))
    , _gridPaddingLabel(_("Grid Padding:"), Gtk::ALIGN_START)
    , _gridPaddingCrossLabel(_(" x "))
    , _gridOriginLabel(_("Grid Origin:"), Gtk::ALIGN_START)
    , _gridOriginCommaLabel(_(" , "))
    , _updatingValues(false)
{
    widget.set_row_spacing(DEFAULT_ROW_SPACING);

    _imageFilenameEntry.set_sensitive(false);

    _imageFilenameBox.add(_imageFilenameEntry);
    _imageFilenameBox.add(_imageFilenameButton);

    widget.attach(_imageFilenameLabel, 0, 0, 1, 1);
    widget.attach(_imageFilenameBox, 1, 0, 3, 1);

    _transparentColorEntry.set_sensitive(false);
    _transparentColorBox.add(_transparentColorEntry);
    _transparentColorBox.add(_transparentColorButton);

    widget.attach(_transparentColorLabel, 0, 1, 1, 1);
    widget.attach(_transparentColorBox, 1, 1, 3, 1);

    widget.attach(_gridFrameSizeLabel, 0, 3, 1, 1);
    widget.attach(_gridFrameSizeSpinButtons.widthSpin, 1, 3, 1, 1);
    widget.attach(_gridFrameSizeCrossLabel, 2, 3, 1, 1);
    widget.attach(_gridFrameSizeSpinButtons.heightSpin, 3, 3, 1, 1);

    widget.attach(_gridOffsetLabel, 0, 4, 1, 1);
    widget.attach(_gridOffsetSpinButtons.xSpin, 1, 4, 1, 1);
    widget.attach(_gridOffsetCommaLabel, 2, 4, 1, 1);
    widget.attach(_gridOffsetSpinButtons.ySpin, 3, 4, 1, 1);

    widget.attach(_gridPaddingLabel, 0, 5, 1, 1);
    widget.attach(_gridPaddingSpinButtons.widthSpin, 1, 5, 1, 1);
    widget.attach(_gridPaddingCrossLabel, 2, 5, 1, 1);
    widget.attach(_gridPaddingSpinButtons.heightSpin, 3, 5, 1, 1);

    widget.attach(_gridOriginLabel, 0, 6, 1, 1);
    widget.attach(_gridOriginSpinButtons.xSpin, 1, 6, 1, 1);
    widget.attach(_gridOriginCommaLabel, 2, 6, 1, 1);
    widget.attach(_gridOriginSpinButtons.ySpin, 3, 6, 1, 1);

    updateGuiValues();

    /**
     * SLOTS
     */

    _imageFilenameButton.signal_clicked().connect([this](void) {
        if (_frameSet && !_updatingValues) {
            // ::TODO implement::
        }
    });

    _gridFrameSizeSpinButtons.signal_valueChanged.connect([this](void) {
        if (_frameSet && !_updatingValues) {
            frameSet_Grid_setFrameSize(_frameSet, _gridFrameSizeSpinButtons.value());
        }
    });

    _gridOffsetSpinButtons.signal_valueChanged.connect([this](void) {
        if (_frameSet && !_updatingValues) {
            frameSet_Grid_setOffset(_frameSet, _gridOffsetSpinButtons.value());
        }
    });

    _gridPaddingSpinButtons.signal_valueChanged.connect([this](void) {
        if (_frameSet && !_updatingValues) {
            frameSet_Grid_setPadding(_frameSet, _gridPaddingSpinButtons.value());
        }
    });

    _gridOriginSpinButtons.signal_valueChanged.connect([this](void) {
        if (_frameSet && !_updatingValues) {
            frameSet_Grid_setOrigin(_frameSet, _gridOriginSpinButtons.value());
        }
    });

    /** FrameSet Updated signal */
    Signals::frameSetChanged.connect([this](const std::shared_ptr<SI::FrameSet> frameSet) {
        if (_frameSet == frameSet) {
            updateGuiValues();
        }
    });
}

void FrameSetPropertiesEditor::updateGuiValues()
{
    if (_frameSet) {
        _updatingValues = true;

        _imageFilenameEntry.set_text(_frameSet->imageFilename());
        _imageFilenameEntry.set_position(-1); // right align text

        if (_frameSet->transparentColorValid()) {
            auto transparent = _frameSet->transparentColor();

            auto c = Glib::ustring::format(std::setfill(L'0'), std::setw(6), std::hex,
                                           transparent.rgb());
            _transparentColorEntry.set_text(c);

            Gdk::RGBA rgba;
            rgba.set_rgba(transparent.red / 255.0, transparent.green / 255.0, transparent.blue / 255.0, 1.0);
            _transparentColorButton.get_children()[0]->override_background_color(rgba);
        }
        else {
            _transparentColorEntry.set_text("");
            _transparentColorButton.get_children()[0]->unset_background_color();
        }

        _gridFrameSizeSpinButtons.set_value(_frameSet->grid().frameSize());
        _gridOffsetSpinButtons.set_value(_frameSet->grid().offset());
        _gridPaddingSpinButtons.set_value(_frameSet->grid().padding());
        _gridOriginSpinButtons.set_value(_frameSet->grid().origin());

        _updatingValues = false;

        widget.set_sensitive(true);
    }
    else {
        static const upoint zeroPoint = { 0, 0 };
        static const usize zeroSize = { 0, 0 };

        _imageFilenameEntry.set_text("");
        _transparentColorEntry.set_text("");
        _transparentColorButton.get_children()[0]->unset_background_color();

        _gridFrameSizeSpinButtons.set_value(zeroSize);
        _gridOffsetSpinButtons.set_value(zeroPoint);
        _gridPaddingSpinButtons.set_value(zeroSize);
        _gridOriginSpinButtons.set_value(zeroPoint);

        widget.set_sensitive(false);
    }
}
