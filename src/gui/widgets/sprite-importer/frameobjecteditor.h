#ifndef _UNTECH_GUI_WIDGETS_SPRITEIMPORTER_FRAMEOBJECTEDITOR_H_
#define _UNTECH_GUI_WIDGETS_SPRITEIMPORTER_FRAMEOBJECTEDITOR_H_

#include "selection.h"
#include "gui/widgets/common/aabb.h"

#include <gtkmm.h>

namespace UnTech {
namespace Widgets {
namespace SpriteImporter {

class FrameObjectEditor {
public:
    FrameObjectEditor(Selection& selection);

protected:
    void updateGuiValues();

public:
    Gtk::Grid widget;

private:
    Selection& _selection;

    UpointSpinButtons _locationSpinButtons;
    Gtk::ComboBoxText _sizeCombo;

    Gtk::Label _locationLabel, _locationCommaLabel, _sizeLabel;

    bool _updatingValues;
};
}
}
}

#endif
