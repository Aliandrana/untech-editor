#include "framesetgraphicaleditor.h"
#include "signals.h"
#include "../common/cr_rgba.h"

using namespace UnTech::Widgets::SpriteImporter;
namespace SI = UnTech::SpriteImporter;

FrameSetGraphicalEditor::FrameSetGraphicalEditor()
    : Gtk::DrawingArea()
    , _frameSet(nullptr)
    , _selectedFrame(nullptr)
    , _selectedItem(nullptr)
    , _zoomX(3.0)
    , _zoomY(3.0)
    , _frameSetImage()
{
    // SLOTS
    // =====
    Signals::frameListChanged.connect(sigc::hide(sigc::mem_fun(this, &FrameSetGraphicalEditor::queue_draw)));
    Signals::frameObjectListChanged.connect(sigc::hide(sigc::mem_fun(this, &FrameSetGraphicalEditor::queue_draw)));
    Signals::actionPointListChanged.connect(sigc::hide(sigc::mem_fun(this, &FrameSetGraphicalEditor::queue_draw)));
    Signals::entityHitboxListChanged.connect(sigc::hide(sigc::mem_fun(this, &FrameSetGraphicalEditor::queue_draw)));

    Signals::frameSetGridChanged.connect([this](const std::shared_ptr<SI::FrameSet> frameSet) {
        if (frameSet == _frameSet) {
            queue_draw();
        }
    });
    Signals::frameSizeChanged.connect([this](const std::shared_ptr<SI::Frame> frame) {
        if (frame && frame->frameSet().lock() == _frameSet) {
            queue_draw();
        }
    });
    Signals::frameObjectChanged.connect([this](const std::shared_ptr<SI::FrameObject> obj) {
        if (obj && obj->frame().lock()->frameSet().lock() == _frameSet) {
            queue_draw();
        }
    });
    Signals::actionPointLocationChanged.connect([this](const std::shared_ptr<SI::ActionPoint> ap) {
        if (ap && ap->frame().lock()->frameSet().lock() == _frameSet) {
            queue_draw();
        }
    });
    Signals::entityHitboxLocationChanged.connect([this](const std::shared_ptr<SI::EntityHitbox> eh) {
        if (eh && eh->frame().lock()->frameSet().lock() == _frameSet) {
            queue_draw();
        }
    });
}

// ::TODO call on signal_frameSetImageChanged signal::
void FrameSetGraphicalEditor::resizeWidget()
{
    if (_frameSet && !_frameSet->image().empty()) {
        const auto imgSize = _frameSet->image().size();

        this->set_size_request(imgSize.width * _zoomX,
                               imgSize.height * _zoomY);
    }
    else {
        this->set_size_request(-1, -1);
    }

    queue_draw();
}

void FrameSetGraphicalEditor::loadAndScaleImage()
{
    if (_frameSet) {
        const auto& img = _frameSet->image();

        if (!img.empty()) {
            int width = img.size().width * _zoomX;
            int height = img.size().height * _zoomY;

            auto pixbuf = Gdk::Pixbuf::create_from_data(reinterpret_cast<const guint8*>(img.data()),
                                                        Gdk::Colorspace::COLORSPACE_RGB, true, 8,
                                                        img.size().width, img.size().height,
                                                        img.size().width * 4);

            _frameSetImage = pixbuf->scale_simple(width, height, Gdk::InterpType::INTERP_NEAREST);
        }
        else {
            // show a gray tile
            _frameSetImage = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, 16, 16);
            _frameSetImage->fill(0x80808080);
        }
    }
    else {
        // show an empty pixel
        _frameSetImage = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, 1, 1);
        _frameSetImage->fill(0);
    }

    queue_draw();
}

inline void FrameSetGraphicalEditor::cr_zoom_rectangle(const Cairo::RefPtr<Cairo::Context>& cr,
                                                       unsigned x, unsigned y,
                                                       unsigned width, unsigned height)
{
    cr->rectangle(x * _zoomX, y * _zoomY,
                  width * _zoomX, height * _zoomY);
}

bool FrameSetGraphicalEditor::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    // ::TODO move::
    const double FRAME_BORDER_WIDTH = 1;
    const cr_rgba frameBorderColor = { 0.5, 0.5, 0.5, 0.5 };
    const cr_rgba frameSelectedClipColor = { 0.7, 0.7, 0.7, 0.7 };

    const double TILE_HITBOX_WIDTH = 1;
    const cr_rgba frameTileHitboxColor = { 0.8, 0.0, 0.0, 0.8 };

    const double FRAME_OBJECT_WIDTH = 0.5;
    const cr_rgba frameObjectColor = { 0.3, 0.9, 0.3, 0.8 };

    const double ACTION_POINT_WIDTH = 1.5;
    const double ACTION_POINT_SIZE = 1;
    const cr_rgba actionPointColor = { 0.7, 0.7, 0.2, 0.8 };

    const double ENTITY_HITBOX_WIDTH = 1;
    const cr_rgba entityHitboxColor = { 0.2, 0.0, 0.8, 0.8 };

    if (_frameSet == nullptr) {
        return true;
    }

    cr->save();

    Gdk::Cairo::set_source_pixbuf(cr, _frameSetImage, 0, 0);
    cr->paint();

    for (const auto frameIt : _frameSet->frames()) {
        const auto frame = frameIt.second;
        const auto loc = frame->location();

        cr_zoom_rectangle(cr, loc.x, loc.y, loc.width, loc.height);

        frameBorderColor.apply(cr);
        cr->set_line_width(FRAME_BORDER_WIDTH);
        cr->stroke();

        if (frame->solid()) {
            const auto& hb = frame->tileHitbox();

            cr_zoom_rectangle(cr, loc.x + hb.x, loc.y + hb.y,
                              hb.width, hb.height);
            frameTileHitboxColor.apply(cr);
            cr->set_line_width(TILE_HITBOX_WIDTH);
            cr->stroke();
        }

        for (const auto ap : frame->actionPoints()) {
            const auto aLoc = ap->location();

            double aWidth = ACTION_POINT_SIZE * _zoomX;
            double aHeight = ACTION_POINT_SIZE * _zoomY;
            double x = (loc.x + aLoc.x) * _zoomX;
            double y = (loc.y + aLoc.y) * _zoomY;

            cr->move_to(x, y - aHeight);
            cr->line_to(x, y + aHeight);
            cr->move_to(x - aWidth, y);
            cr->line_to(x + aWidth, y);

            // ::SHOULDO different color lines depending on type::
            actionPointColor.apply(cr);
            cr->set_line_width(ACTION_POINT_WIDTH);
            cr->stroke();
        }

        for (const auto eh : frame->entityHitboxes()) {
            const auto aabb = eh->aabb();

            cr_zoom_rectangle(cr, loc.x + aabb.x, loc.y + aabb.y,
                              aabb.width, aabb.height);

            // ::SHOULDO different color lines depending on type::
            entityHitboxColor.apply(cr);
            cr->set_line_width(ENTITY_HITBOX_WIDTH);
            cr->stroke();
        }

        for (const auto obj : frame->objects()) {
            const auto oloc = obj->location();

            cr_zoom_rectangle(cr, loc.x + oloc.x, loc.y + oloc.y,
                              obj->sizePx(), obj->sizePx());
            frameObjectColor.apply(cr);
            cr->set_line_width(FRAME_OBJECT_WIDTH);
            cr->stroke();
        }
    }

    if (_selectedFrame) {
        // highlight everything that is not the selected frame.
        const auto sLoc = _selectedFrame->location();
        auto allocation = get_allocation();
        const unsigned aWidth = allocation.get_width();
        const unsigned aHeight = allocation.get_height();

        cr_zoom_rectangle(cr, 0, 0, sLoc.x, aHeight);
        cr_zoom_rectangle(cr, 0, 0, aWidth, sLoc.y);
        cr_zoom_rectangle(cr, sLoc.right(), 0, aWidth - sLoc.right(), aHeight);
        cr_zoom_rectangle(cr, 0, sLoc.bottom(), aWidth, aHeight - sLoc.bottom());

        frameSelectedClipColor.apply(cr);
        cr->fill();
    }

    cr->restore();

    return true;
}

void FrameSetGraphicalEditor::setFrameSet(std::shared_ptr<SI::FrameSet> frameSet)
{
    if (_frameSet != frameSet) {
        _frameSet = frameSet;
        _selectedFrame = nullptr;
        _selectedItem = nullptr;

        loadAndScaleImage();
        resizeWidget();
    }
}

void FrameSetGraphicalEditor::setFrame(std::shared_ptr<SI::Frame> frame)
{
    if (_selectedFrame != frame) {
        _selectedFrame = frame;
        _selectedItem = nullptr;

        queue_draw();
    }
}

void FrameSetGraphicalEditor::setFrameObject(std::shared_ptr<SI::FrameObject> frameObject)
{
    _selectedItem = frameObject.get();
    queue_draw();
}

void FrameSetGraphicalEditor::setActionPoint(std::shared_ptr<SI::ActionPoint> actionPoint)
{
    _selectedItem = actionPoint.get();
    queue_draw();
}

void FrameSetGraphicalEditor::setEntityHitbox(std::shared_ptr<SI::EntityHitbox> entityHitbox)
{
    _selectedItem = entityHitbox.get();
    queue_draw();
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

void FrameSetGraphicalEditor::setZoom(double x, double y)
{
    if (_zoomX != x || _zoomY != y) {
        _zoomX = limit(x, 1.0, 10.0);
        _zoomY = limit(y, 1.0, 10.0);

        resizeWidget();
        loadAndScaleImage();
    }
}
