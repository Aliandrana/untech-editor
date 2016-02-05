#ifndef _UNTECH_MODELS_SPRITEIMPORTER_FRAMEOBJECT_H
#define _UNTECH_MODELS_SPRITEIMPORTER_FRAMEOBJECT_H

#include "../common/aabb.h"
#include <memory>

namespace UnTech {
namespace SpriteImporter {

class Frame;

class FrameObject {

public:
    enum class ObjectSize {
        SMALL = 8,
        LARGE = 16
    };

    FrameObject() = delete;
    FrameObject(std::shared_ptr<Frame> frame);
    FrameObject(const FrameObject& object, std::shared_ptr<Frame> frame);

    upoint location() const { return _location; }
    ObjectSize size() const { return _size; }

    unsigned sizePx() const { return (unsigned)_size; }

    void setLocation(const upoint& location);
    void setSize(ObjectSize size);

    /**
     * bottom left point of the frame.
     */
    upoint bottomLeft() const
    {
        return { _location.x + sizePx(), _location.y + sizePx() };
    }

protected:
    void validateLocation();

private:
    std::weak_ptr<Frame> _frame;
    upoint _location;
    ObjectSize _size;
};
}
}

#endif
