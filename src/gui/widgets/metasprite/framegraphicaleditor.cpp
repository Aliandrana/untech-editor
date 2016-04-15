#include "framegraphicaleditor.h"
#include "signals.h"
#include "gui/undo/actionhelper.h"
#include "../common/cr_rgba.h"

#include <cmath>

using namespace UnTech::Widgets::MetaSprite;

SIMPLE_UNDO_ACTION(frameObject_setLocation,
                   MS::FrameObject, UnTech::ms8point, location, setLocation,
                   Signals::frameObjectChanged,
                   "Move Frame Object")

SIMPLE_UNDO_ACTION(actionPoint_setLocation,
                   MS::ActionPoint, UnTech::ms8point, location, setLocation,
                   Signals::actionPointChanged,
                   "Move Action Point")

SIMPLE_UNDO_ACTION(entityHitbox_setAabb,
                   MS::EntityHitbox, UnTech::ms8rect, aabb, setAabb,
                   Signals::entityHitboxChanged,
                   "Move Entity Hitbox")

FrameGraphicalEditor::FrameGraphicalEditor(Selection& selection)
    : Gtk::DrawingArea()
    , _zoomX(3.0)
    , _zoomY(3.0)
    , _xOffset(64)
    , _yOffset(64)
    , _displayZoom(NAN)
    , _selection(selection)
{
    add_events(Gdk::BUTTON_PRESS_MASK
               | Gdk::BUTTON_RELEASE_MASK
               | Gdk::ENTER_NOTIFY_MASK
               | Gdk::LEAVE_NOTIFY_MASK
               | Gdk::BUTTON1_MOTION_MASK);

    // SLOTS
    // =====
    Signals::frameObjectListChanged.connect(sigc::hide(sigc::mem_fun(this, &FrameGraphicalEditor::queue_draw)));
    Signals::actionPointListChanged.connect(sigc::hide(sigc::mem_fun(this, &FrameGraphicalEditor::queue_draw)));
    Signals::entityHitboxListChanged.connect(sigc::hide(sigc::mem_fun(this, &FrameGraphicalEditor::queue_draw)));

    _selection.signal_selectionChanged.connect([this](void) {
        // reset action
        _action.state = Action::NONE;
        queue_draw();
    });

    Signals::frameChanged.connect([this](const std::shared_ptr<MS::Frame> frame) {
        if (frame == _selection.frame()) {
            queue_draw();
        }
    });
    Signals::frameObjectChanged.connect([this](const std::shared_ptr<MS::FrameObject> obj) {
        if (obj && obj->frame() == _selection.frame()) {
            queue_draw();
        }
    });
    Signals::actionPointChanged.connect([this](const std::shared_ptr<MS::ActionPoint> ap) {
        if (ap && ap->frame() == _selection.frame()) {
            queue_draw();
        }
    });
    Signals::entityHitboxChanged.connect([this](const std::shared_ptr<MS::EntityHitbox> eh) {
        if (eh && eh->frame() == _selection.frame()) {
            queue_draw();
        }
    });
}

bool FrameGraphicalEditor::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    // ::TODO move::
    const double ITEM_WIDTH = 1.0;
    const double ACTION_POINT_SIZE = 1.5;
    const cr_rgba frameTileHitboxColor = { 0.8, 0.0, 0.0, 0.7 };
    const cr_rgba frameObjectColor = { 0.3, 0.9, 0.3, 0.7 };
    const cr_rgba actionPointColor = { 0.7, 1.0, 0.5, 0.95 };
    const cr_rgba entityHitboxColor = { 0.2, 0.0, 0.8, 0.7 };
    const cr_rgba selectionInnerColor = { 1.0, 1.0, 1.0, 1.0 };
    const cr_rgba selectionOuterColor = { 0.0, 0.0, 0.0, 1.0 };
    const cr_rgba selectionDragColor = { 0.5, 0.5, 0.5, 0.5 };

    const double ORIGIN_WIDTH = 1.0;
    const double ORIGIN_DASH = 3.0;
    const cr_rgba originColor1 = { 0.0, 0.0, 0.0, 0.2 };
    const cr_rgba originColor2 = { 1.0, 1.0, 1.0, 0.2 };

    if (_selection.frame() == nullptr) {
        return true;
    }

    if (std::isnan(_displayZoom)) {
        auto screen = get_screen();

        if (screen) {
            _displayZoom = std::floor(screen->get_width() / 1500) + 1.0;
        }
        else {
            _displayZoom = 1.0;
        }
    }

    auto draw_rectangle = [this, cr](unsigned x, unsigned y, unsigned width, unsigned height) {
        cr->rectangle((x + _xOffset) * _zoomX, (y + _yOffset) * _zoomY,
                      width * _zoomX, height * _zoomY);
    };

    cr->save();
    cr->set_antialias(Cairo::ANTIALIAS_NONE);
    cr->scale(_displayZoom, _displayZoom);

    if (_displayZoom > 1.0) {
        cr->set_antialias(Cairo::ANTIALIAS_DEFAULT);
    }

    const auto frame = _selection.frame();

    cr->set_line_width(ITEM_WIDTH);

    if (frame->solid()) {
        const auto& hb = frame->tileHitbox();

        draw_rectangle(hb.x, hb.y,
                       hb.width, hb.height);
        frameTileHitboxColor.apply(cr);
        cr->stroke();
    }

    for (const auto eh : frame->entityHitboxes()) {
        const auto aabb = eh->aabb();

        draw_rectangle(aabb.x, aabb.y,
                       aabb.width, aabb.height);

        // ::SHOULDO different color lines depending on type::
        entityHitboxColor.apply(cr);
        cr->stroke();
    }

    for (const auto obj : frame->objects()) {
        const auto oloc = obj->location();

        draw_rectangle(oloc.x, oloc.y,
                       obj->sizePx(), obj->sizePx());
        frameObjectColor.apply(cr);
        cr->stroke();
    }

    for (const auto ap : frame->actionPoints()) {
        const auto aLoc = ap->location();

        double aWidth = ACTION_POINT_SIZE * _zoomX / 2;
        double aHeight = ACTION_POINT_SIZE * _zoomY / 2;
        double x = (_xOffset + aLoc.x + 0.5) * _zoomX;
        double y = (_yOffset + aLoc.y + 0.5) * _zoomY;

        cr->move_to(x, y - aHeight);
        cr->line_to(x, y + aHeight);
        cr->move_to(x - aWidth, y);
        cr->line_to(x + aWidth, y);

        // ::SHOULDO different color lines depending on type::
        actionPointColor.apply(cr);
        cr->stroke();
    }

    // Draw Origin
    {
        cr->save();

        static const std::vector<double> originDash({ ORIGIN_DASH, ORIGIN_DASH });

        auto allocation = get_allocation();
        const unsigned aWidth = allocation.get_width();
        const unsigned aHeight = allocation.get_height();

        double x = _xOffset * _zoomX;
        double y = _yOffset * _zoomY;

        cr->move_to(x, y);
        cr->line_to(0, y);

        cr->move_to(x, y);
        cr->line_to(aWidth, y);

        cr->move_to(x, y);
        cr->line_to(x, 0);

        cr->move_to(x, y);
        cr->line_to(x, aHeight);

        cr->set_line_width(ORIGIN_WIDTH);

        originColor1.apply(cr);
        cr->set_dash(originDash, ORIGIN_DASH / 2);
        cr->stroke_preserve();

        originColor2.apply(cr);
        cr->set_dash(originDash, ORIGIN_DASH + ORIGIN_DASH / 2);
        cr->stroke();

        cr->restore();
    }

    auto draw_selected_rectangle = [&](unsigned x, unsigned y,
                                       unsigned width, unsigned height) {
        cr->set_line_width(1);

        const double zX = (x + _xOffset) * _zoomX + 1;
        const double zY = (y + _yOffset) * _zoomY + 1;
        const double zWidth = width * _zoomX - 1;
        const double zHeight = height * _zoomY - 1;

        cr->rectangle(zX, zY, zWidth, zHeight);
        cr->stroke();

        selectionInnerColor.apply(cr);
        cr->rectangle(zX + 1, zY + 1, zWidth - 2, zHeight - 2);
        cr->stroke();

        selectionOuterColor.apply(cr);
        cr->rectangle(zX - 1, zY - 1, zWidth + 2, zHeight + 2);
        cr->stroke();
    };

    switch (_selection.type()) {
    case Selection::Type::NONE:
        break;

    case Selection::Type::FRAME_OBJECT:
        if (_selection.frameObject()) {
            const auto oLoc = _selection.frameObject()->location();
            const unsigned oSize = _selection.frameObject()->sizePx();

            // ::TODO draw Frame Object::

            frameObjectColor.apply(cr);
            draw_selected_rectangle(oLoc.x, oLoc.y, oSize, oSize);
        }
        break;

    case Selection::Type::ENTITY_HITBOX:
        if (_selection.entityHitbox()) {
            const auto aabb = _selection.entityHitbox()->aabb();

            // ::SHOULDO different color lines depending on type::
            entityHitboxColor.apply(cr);
            draw_selected_rectangle(aabb.x, aabb.y, aabb.width, aabb.height);
        }
        break;

    case Selection::Type::ACTION_POINT:
        if (_selection.actionPoint()) {
            const auto aLoc = _selection.actionPoint()->location();

            cr->save();

            double aWidth = ACTION_POINT_SIZE * _zoomX / 2;
            double aHeight = ACTION_POINT_SIZE * _zoomY / 2;
            double x = (aLoc.x + _xOffset + 0.5) * _zoomX;
            double y = (aLoc.y + _yOffset + 0.5) * _zoomY;

            cr->move_to(x, y - aHeight - 1.0);
            cr->line_to(x, y + aHeight + 1.0);
            cr->move_to(x - aWidth - 1.0, y);
            cr->line_to(x + aWidth + 1.0, y);

            selectionOuterColor.apply(cr);
            cr->set_line_width(3.0);
            cr->stroke();

            cr->move_to(x, y - aHeight);
            cr->line_to(x, y + aHeight);
            cr->move_to(x - aWidth, y);
            cr->line_to(x + aWidth, y);

            selectionInnerColor.apply(cr);
            cr->set_line_width(1.0);
            cr->stroke();

            cr->restore();
        }
        break;
    }

    if (_action.state == Action::DRAG) {
        const ms8rect aabb = _action.dragAabb;

        // ::SHOULDDO checkerboard pattern::

        draw_rectangle(aabb.x, aabb.y, aabb.width, aabb.height);
        selectionDragColor.apply(cr);
        cr->fill();
    }

    cr->restore();

    return true;
}

bool FrameGraphicalEditor::on_button_press_event(GdkEventButton* event)
{
    if (_selection.frame() == nullptr) {
        return false;
    }

    if (event->button == 1) {
        if (_action.state == Action::NONE) {
            auto allocation = get_allocation();

            int mouseX = std::lround((event->x - allocation.get_x()) / (_zoomX * _displayZoom)) - _xOffset;
            int mouseY = std::lround((event->y - allocation.get_y()) / (_zoomY * _displayZoom)) - _yOffset;

            _action.canDrag = false;

            _action.state = Action::CLICK;
            _action.pressLocation = ms8point(mouseX, mouseY);
            ms8rect aabb;

            switch (_selection.type()) {
            case Selection::Type::NONE:
                break;

            case Selection::Type::FRAME_OBJECT:
                if (_selection.frameObject()) {
                    const auto fo = _selection.frameObject();

                    if (fo->frame()) {
                        const auto foLoc = fo->location();
                        aabb = ms8rect(foLoc.x, foLoc.y, fo->sizePx(), fo->sizePx());
                        _action.canDrag = true;
                    }
                }
                break;

            case Selection::Type::ACTION_POINT:
                if (_selection.actionPoint()) {
                    const auto ap = _selection.actionPoint();

                    if (ap->frame()) {
                        const auto apLoc = ap->location();
                        aabb = ms8rect(apLoc.x, apLoc.y, 1, 1);
                        _action.canDrag = true;
                    }
                }
                break;

            case Selection::Type::ENTITY_HITBOX:
                if (_selection.entityHitbox()) {
                    const auto eh = _selection.entityHitbox();

                    if (eh->frame()) {
                        aabb = eh->aabb();
                        _action.canDrag = true;
                    }
                }
                break;
            }

            if (_action.canDrag) {
                _action.dragAabb = aabb;

                const ms8point fm(mouseX, mouseY);

                if (_selection.type() == Selection::Type::ENTITY_HITBOX) {

                    _action.resizeLeft = fm.x == aabb.left();
                    _action.resizeRight = fm.x == aabb.right();
                    _action.resizeTop = fm.y == aabb.top();
                    _action.resizeBottom = fm.y == aabb.bottom();

                    _action.resize = _action.resizeLeft | _action.resizeRight
                                     | _action.resizeTop | _action.resizeBottom;
                }
                else {
                    _action.resize = false;
                }

                // make sure click is inside the item
                _action.canDrag = aabb.contains(fm) || _action.resize;
            }
        }
    }
    return true;
}

bool FrameGraphicalEditor::on_motion_notify_event(GdkEventMotion* event)
{
    if (_selection.frame() == nullptr) {
        _action.state = Action::NONE;
        return true;
    }

    if (_action.state != Action::NONE) {
        auto allocation = get_allocation();

        int mouseX = std::lround((event->x - allocation.get_x()) / (_zoomX * _displayZoom)) - _xOffset;
        int mouseY = std::lround((event->y - allocation.get_y()) / (_zoomY * _displayZoom)) - _yOffset;

        ms8point mouse(mouseX, mouseY);

        if (_action.state == Action::CLICK && _action.canDrag) {
            if (_action.pressLocation != mouse) {
                _action.state = Action::DRAG;
                _action.previousLocation = _action.pressLocation;

                update_pointer_cursor();
            }
        }

        if (_action.state == Action::DRAG) {
            // move dragAbbb to new location.
            if (_action.previousLocation != mouse) {
                ms8rect aabb = _action.dragAabb;

                if (!_action.resize) {
                    // move
                    aabb.x += mouse.x - _action.previousLocation.x;
                    aabb.y += mouse.y - _action.previousLocation.y;
                }
                else {
                    // resize
                    int rx = mouse.x;
                    if (_action.resizeLeft) {
                        if (rx >= aabb.right()) {
                            rx = aabb.right() - 1;
                        }
                        aabb.width = aabb.right() - rx;
                        aabb.x = rx;
                    }
                    else if (_action.resizeRight) {
                        if (rx <= aabb.left()) {
                            rx = aabb.left() + 1;
                        }
                        aabb.width = rx - aabb.x;
                    }

                    int ry = mouse.y;
                    if (_action.resizeTop) {
                        if (ry >= aabb.bottom()) {
                            ry = aabb.bottom() - 1;
                        }
                        aabb.height = aabb.bottom() - ry;
                        aabb.y = ry;
                    }
                    else if (_action.resizeBottom) {
                        if (ry <= aabb.top()) {
                            ry = aabb.top() + 1;
                        }
                        aabb.height = ry - aabb.y;
                    }
                }

                if (_action.dragAabb != aabb) {
                    _action.dragAabb = aabb;
                    queue_draw();
                }

                _action.previousLocation = mouse;
            }
        }
    }
    return true;
}

bool FrameGraphicalEditor::on_button_release_event(GdkEventButton* event)
{
    if (_selection.frame() == nullptr) {
        return false;
    }

    if (event->button == 1) {
        auto allocation = get_allocation();

        int x = std::lround((event->x - allocation.get_x()) / (_zoomX * _displayZoom)) - _xOffset;
        int y = std::lround((event->y - allocation.get_y()) / (_zoomY * _displayZoom)) - _yOffset;

        ms8point mouse(x, y);

        switch (_action.state) {
        case Action::CLICK:
            handleRelease_Click(mouse);
            break;

        case Action::DRAG:
            handleRelease_Drag();
            break;

        case Action::NONE:
            break;
        }
    }
    return true;
}

void FrameGraphicalEditor::handleRelease_Click(const ms8point& mouse)
{
    _action.state = Action::NONE;

    // only select item if mouse didn't move.
    if (mouse != _action.pressLocation) {
        return;
    }

    /*
     * Select a given item.
     *
     * This code cycles through the given selections.
     * On click, the next item is selected. If the last item
     * was the previously selected one then the first match
     * is selected.
     */
    struct SelHandler {
        Selection::Type type = Selection::Type::NONE;
        std::shared_ptr<MS::FrameObject> frameObject = nullptr;
        std::shared_ptr<MS::ActionPoint> actionPoint = nullptr;
        std::shared_ptr<MS::EntityHitbox> entityHitbox = nullptr;
    };
    SelHandler current;
    SelHandler firstMatch;

    const auto sFrame = _selection.frame();

    for (const auto obj : sFrame->objects()) {
        const auto loc = obj->location();

        if (mouse.x >= loc.x && mouse.x < (loc.x + (int)obj->sizePx())
            && mouse.y >= loc.y && mouse.y < (loc.y + (int)obj->sizePx())) {
            if (current.type == Selection::Type::NONE) {
                current.frameObject = obj;
                current.type = Selection::Type::FRAME_OBJECT;

                if (firstMatch.type == Selection::Type::NONE) {
                    firstMatch.frameObject = obj;
                    firstMatch.type = Selection::Type::FRAME_OBJECT;
                }
            }
            if (obj == _selection.frameObject()
                && _selection.type() == Selection::Type::FRAME_OBJECT) {
                current.type = Selection::Type::NONE;
            }
        }
    }

    for (const auto ap : sFrame->actionPoints()) {
        const auto loc = ap->location();

        if (mouse == loc) {
            if (current.type == Selection::Type::NONE) {
                current.actionPoint = ap;
                current.type = Selection::Type::ACTION_POINT;

                if (firstMatch.type == Selection::Type::NONE) {
                    firstMatch.actionPoint = ap;
                    firstMatch.type = Selection::Type::ACTION_POINT;
                }
            }
            if (ap == _selection.actionPoint()
                && _selection.type() == Selection::Type::ACTION_POINT) {
                current.type = Selection::Type::NONE;
            }
        }
    }

    for (const auto eh : sFrame->entityHitboxes()) {
        const auto aabb = eh->aabb();

        if (aabb.contains(mouse)) {
            if (current.type == Selection::Type::NONE) {
                current.entityHitbox = eh;
                current.type = Selection::Type::ENTITY_HITBOX;

                if (firstMatch.type == Selection::Type::NONE) {
                    firstMatch.entityHitbox = eh;
                    firstMatch.type = Selection::Type::ENTITY_HITBOX;
                }
            }
            if (eh == _selection.entityHitbox()
                && _selection.type() == Selection::Type::ENTITY_HITBOX) {
                current.type = Selection::Type::NONE;
            }
        }
    }

    if (current.type == Selection::Type::NONE) {
        // handle wrap around.
        current = firstMatch;
    }

    switch (current.type) {
    case Selection::Type::NONE:
        _selection.unselectAll();
        break;

    case Selection::Type::FRAME_OBJECT:
        _selection.setFrameObject(current.frameObject);
        break;

    case Selection::Type::ACTION_POINT:
        _selection.setActionPoint(current.actionPoint);
        break;

    case Selection::Type::ENTITY_HITBOX:
        _selection.setEntityHitbox(current.entityHitbox);
        break;
    }
}

void FrameGraphicalEditor::handleRelease_Drag()
{
    _action.state = Action::NONE;
    update_pointer_cursor();

    const auto aabb = _action.dragAabb;

    switch (_selection.type()) {
    case Selection::Type::NONE:
        break;

    case Selection::Type::FRAME_OBJECT:
        if (_selection.frameObject()) {
            frameObject_setLocation(_selection.frameObject(),
                                    ms8point(aabb.x, aabb.y));
        }
        break;

    case Selection::Type::ACTION_POINT:
        if (_selection.actionPoint()) {
            actionPoint_setLocation(_selection.actionPoint(),
                                    ms8point(aabb.x, aabb.y));
        }
        break;

    case Selection::Type::ENTITY_HITBOX:
        if (_selection.entityHitbox()) {
            entityHitbox_setAabb(_selection.entityHitbox(), aabb);
        }
        break;
    }

    queue_draw();
}

bool FrameGraphicalEditor::on_enter_notify_event(GdkEventCrossing*)
{
    update_pointer_cursor();
    return true;
}

void FrameGraphicalEditor::update_pointer_cursor()
{
    auto win = get_window();
    if (win) {
        switch (_action.state) {
        case Action::NONE:
        case Action::CLICK:
            win->set_cursor();
            break;

        case Action::DRAG:
            if (!_action.resize) {
                win->set_cursor(Gdk::Cursor::create(get_display(), "move"));
            }
            else {
                bool horizontal = _action.resizeLeft | _action.resizeRight;
                bool vertical = _action.resizeTop | _action.resizeBottom;

                if (horizontal && !vertical) {
                    win->set_cursor(Gdk::Cursor::create(get_display(), "ew-resize"));
                }
                else if (vertical && !horizontal) {
                    win->set_cursor(Gdk::Cursor::create(get_display(), "ns-resize"));
                }
                else if ((_action.resizeLeft && _action.resizeTop)
                         || (_action.resizeRight && _action.resizeBottom)) {
                    win->set_cursor(Gdk::Cursor::create(get_display(), "nwse-resize"));
                }
                else if ((_action.resizeRight && _action.resizeTop)
                         || (_action.resizeLeft && _action.resizeBottom)) {
                    win->set_cursor(Gdk::Cursor::create(get_display(), "nesw-resize"));
                }
            }
            break;
        }
    }
}

bool FrameGraphicalEditor::on_leave_notify_event(GdkEventCrossing*)
{
    auto win = get_window();
    if (win) {
        win->set_cursor();
    }

    return true;
}

inline double limit(double v, double min, double max)
{
    if (v < min) {
        return min;
    }
    else if (v > max) {
        return max;
    }
    else {
        return v;
    }
}

void FrameGraphicalEditor::setZoom(double x, double y)
{
    if (_zoomX != x || _zoomY != y) {
        _zoomX = limit(x, 1.0, 10.0);
        _zoomY = limit(y, 1.0, 10.0);

        queue_draw();
    }
}

void FrameGraphicalEditor::setOffset(int x, int y)
{
    if (_xOffset != x || _yOffset != y) {
        _xOffset = x;
        _yOffset = y;

        queue_draw();
    }
}