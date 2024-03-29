#include "frame.h"
#include "frameobject.h"
#include "actionpoint.h"
#include "entityhitbox.h"
#include "palette.h"
#include "../snes/tileset.hpp"

using namespace UnTech;
using namespace UnTech::MetaSprite;

Frame::Frame(FrameSet& frameSet)
    : _frameSet(frameSet)
    , _objects(*this)
    , _actionPoints(*this)
    , _entityHitboxes(*this)
    , _solid(true)
    , _tileHitbox(-8, -8, 16, 16)
{
}

Frame::Frame(const Frame& frame, FrameSet& frameSet)
    : _frameSet(frameSet)
    , _objects(*this)
    , _actionPoints(*this)
    , _entityHitboxes(*this)
    , _solid(frame._solid)
    , _tileHitbox(frame._tileHitbox)
{
    for (const auto& obj : frame._objects) {
        _objects.clone(obj);
    }
    for (const auto& ap : frame._actionPoints) {
        _actionPoints.clone(ap);
    }
    for (const auto& eh : frame._entityHitboxes) {
        _entityHitboxes.clone(eh);
    }
}

Frame::Boundary Frame::calcBoundary() const
{
    // These numbers are selected so that origin (0, 0) is always visible.
    int left = -1;
    int right = 1;
    int top = -1;
    int bottom = 1;

    for (const FrameObject& obj : _objects) {
        const auto& loc = obj.location();
        const int size = obj.sizePx();

        if (loc.x < left) {
            left = loc.x;
        }
        if (loc.x + size > right) {
            right = loc.x + size;
        }
        if (loc.y < top) {
            top = loc.y;
        }
        if (loc.y + size > bottom) {
            bottom = loc.y + size;
        }
    }

    for (const ActionPoint& ap : _actionPoints) {
        const auto& loc = ap.location();

        if (loc.x < left) {
            left = loc.x;
        }
        if (loc.x > right) {
            right = loc.x;
        }
        if (loc.y < top) {
            top = loc.y;
        }
        if (loc.y > bottom) {
            bottom = loc.y;
        }
    }
    for (const EntityHitbox& eh : _entityHitboxes) {
        const auto& aabb = eh.aabb();

        if (aabb.x < left) {
            left = aabb.x;
        }
        if (aabb.right() > right) {
            right = aabb.right();
        }
        if (aabb.y < top) {
            top = aabb.y;
        }
        if (aabb.bottom() > bottom) {
            bottom = aabb.bottom();
        }
    }

    return { left, top,
             (unsigned)right - left,
             (unsigned)top - bottom };
}

void Frame::draw(Image& image, const Palette& palette, unsigned xOffset, unsigned yOffset) const
{
    for (int order = 0; order < 4; order++) {
        for (auto it = _objects.rbegin(); it != _objects.rend(); ++it) {
            const FrameObject& obj = *it;

            if (obj.order() == order) {
                if (obj.size() == FrameObject::ObjectSize::SMALL) {
                    _frameSet.smallTileset().drawTile(image, palette,
                                                      xOffset + obj.location().x, yOffset + obj.location().y,
                                                      obj.tileId(), obj.hFlip(), obj.vFlip());
                }
                else {
                    _frameSet.largeTileset().drawTile(image, palette,
                                                      xOffset + obj.location().x, yOffset + obj.location().y,
                                                      obj.tileId(), obj.hFlip(), obj.vFlip());
                }
            }
        }
    }
}
