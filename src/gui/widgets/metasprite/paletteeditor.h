#ifndef _UNTECH_GUI_WIDGETS_METASPRITE_PALETTEEDITOR_H_
#define _UNTECH_GUI_WIDGETS_METASPRITE_PALETTEEDITOR_H_

#include "selection.h"
#include "signals.h"
#include "../common/colortogglebutton.h"
#include "models/metasprite/palette.h"
#include "gui/widgets/defaults.h"

#include <gtkmm.h>
#include <glibmm/i18n.h>

namespace UnTech {
namespace Widgets {
namespace MetaSprite {

namespace MS = UnTech::MetaSprite;

class PaletteEditor {
public:
    static const unsigned N_COLORS = 16;

public:
    PaletteEditor(Selection& selection);

    void unselectAllColors();

protected:
    void updateGuiValues();

    void on_color_toggled(int cId);

public:
    Gtk::Grid widget;

    // Propagates the selected color to the tile graphic editor.
    // Parameter is negative if no color is selected.
    sigc::signal<void, int> signal_selectColor();

private:
    Selection& _selection;

    Gtk::ToggleButton _editSelectOption;

    ColorToggleButton _colorButtons[N_COLORS];

    Gtk::Label _paletteLabel;

    bool _updatingValues;
};
}
}
}

#endif
