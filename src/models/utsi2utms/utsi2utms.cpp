#include "utsi2utms.h"
#include "models/metasprite.h"
#include "models/sprite-importer.h"
#include <cassert>
#include <map>
#include <set>
#include <sstream>

const size_t PALETTE_COLORS = 16;

using namespace UnTech;
namespace MS = UnTech::MetaSprite;
namespace SI = UnTech::SpriteImporter;

Utsi2Utms::Utsi2Utms()
    : _errors()
    , _warnings()
    , _hasError(false)
{
}

std::unique_ptr<MS::MetaSpriteDocument> Utsi2Utms::convert(const SI::SpriteImporterDocument* siDocument)
{
    assert(siDocument != nullptr);

    _hasError = false;

    const auto siFrameSet = siDocument->frameSet();
    const auto& image = siFrameSet->image();

    // Validate siFrameSet
    {
        if (image.empty()) {
            addError(siFrameSet, "No Image");
        }
        if (siFrameSet->frames().size() == 0) {
            addError(siFrameSet, "No Frames");
        }
        if (siFrameSet->transparentColorValid() == false) {
            addError(siFrameSet, "Transparent color is invalid");
        }
    }

    if (_hasError) {
        return nullptr;
    }

    auto msDocument = std::make_unique<MS::MetaSpriteDocument>();
    auto msFrameSet = msDocument->frameSet();

    msFrameSet->setName(siFrameSet->name());

    // Build map of rgba to palette color
    // Faster than std::unordered_map, only contains 16 elements
    std::map<rgba, unsigned> colorMap;
    {
        std::set<rgba> colors;

        for (const auto siFrameIt : siFrameSet->frames()) {
            const auto& siFrame = siFrameIt.second;

            if (!image.size().contains(siFrame->location())) {
                addError(siFrame, "Frame not inside image");
                continue;
            }

            for (const auto obj : siFrame->objects()) {
                unsigned lx = siFrame->location().x + obj->location().x;
                unsigned ly = siFrame->location().y + obj->location().y;

                for (unsigned y = 0; y < obj->sizePx(); y++) {
                    const rgba* p = image.scanline(ly + y) + lx;

                    for (unsigned x = 0; x < obj->sizePx(); x++) {
                        colors.insert(*p++);
                    }
                }

                if (colors.size() > PALETTE_COLORS) {
                    addError(siFrameSet, "Too many colors, expected a max of 16");
                    return nullptr;
                }
            }
        }

        auto tIt = colors.find(siFrameSet->transparentColor());
        if (tIt != colors.end()) {
            colors.erase(tIt);
        }
        else {
            addWarning(siFrameSet, "Transparent color is not in frame objects");
        }

        // Verify enough colors after remove transparency
        if (colors.size() > (PALETTE_COLORS - 1)) {
            addError(siFrameSet, "Too many colors, expected a max of 16");
            return nullptr;
        }

        // Store palette in MetaSprite
        // ::TODO handle user supplied palettes::
        {
            auto palette = msFrameSet->palettes().create();

            colorMap.insert({ siFrameSet->transparentColor(), 0 });
            palette->color(0).setRgb(siFrameSet->transparentColor());

            int i = 1;
            for (auto c : colors) {
                colorMap.insert({ c, i });
                palette->color(i).setRgb(c);
                i++;
            }
        }
    }

    if (_hasError) {
        return nullptr;
    }

    // Process frames
    for (const auto frameIt : siFrameSet->frames()) {
        const auto siFrame = frameIt.second;
        const auto siFrameOrigin = siFrame->origin();

        auto msFrame = msFrameSet->frames().create(frameIt.first);

        try {
            for (const auto siObj : siFrame->objects()) {
                auto msObj = msFrame->objects().create();

                // ::TODO find overlapping tiles and store in list::
                // ::TODO warning if large tile is in front of and covers a small tile::

                if (siObj->size() == SI::FrameObject::ObjectSize::SMALL) {
                    msObj->setSize(MS::FrameObject::ObjectSize::SMALL);

                    // ::TODO get tile from image::

                    // ::TODO insert tile data into tileset::
                    msObj->setTileId(0);
                    msObj->setHFlip(false);
                    msObj->setVFlip(false);
                }
                else {
                    msObj->setSize(MS::FrameObject::ObjectSize::LARGE);

                    // ::TODO get tile from image::

                    // ::TODO insert tile data into tileset::
                    msObj->setTileId(0);
                    msObj->setHFlip(false);
                    msObj->setVFlip(false);
                }

                msObj->setLocation(ms8point::createFromOffset(siObj->location(), siFrameOrigin));
            }

            for (const auto siAp : siFrame->actionPoints()) {
                auto msAp = msFrame->actionPoints().create();

                msAp->setLocation(ms8point::createFromOffset(siAp->location(), siFrameOrigin));
                msAp->setParameter(siAp->parameter());
            }

            for (const auto siEh : siFrame->entityHitboxes()) {
                auto msEh = msFrame->entityHitboxes().create();

                msEh->setAabb(ms8rect::createFromOffset(siEh->aabb(), siFrameOrigin));
                msEh->setParameter(siEh->parameter());
            }

            if (siFrame->solid()) {
                msFrame->setSolid(true);
                msFrame->setTileHitbox(ms8rect::createFromOffset(siFrame->tileHitbox(), siFrameOrigin));
            }
            else {
                msFrame->setSolid(false);
            }
        }
        catch (const std::out_of_range& ex) {
            // This should not happen unless the frame is very large,
            // a simple error message will do.
            addError(siFrame, ex.what());
            continue;
        }
    }

    if (_hasError) {
        return nullptr;
    }

    // ::TODO process overlapping tiles::

    return msDocument;
}

void Utsi2Utms::addError(const std::string& message)
{
    _errors.push_back(message);
    _hasError = true;
}

void Utsi2Utms::addError(const std::shared_ptr<SI::FrameSet> frameSet, const std::string& message)
{
    std::stringstream out;

    out << frameSet->name()
        << ": "
        << message;

    _errors.push_back(out.str());
    _hasError = true;
}

void Utsi2Utms::addError(const std::shared_ptr<SI::Frame> frame, const std::string& message)
{
    std::stringstream out;

    const auto fs = frame->frameSet();

    out << fs->name() << "." << fs->frames().getName(frame).first
        << ": " << message;

    _errors.push_back(out.str());
    _hasError = true;
}

void Utsi2Utms::addWarning(const std::string& message)
{
    _warnings.push_back(message);
}

void Utsi2Utms::addWarning(const std::shared_ptr<SI::FrameSet> frameSet, const std::string& message)
{
    std::stringstream out;

    out << frameSet->name() << ": " << message;

    _warnings.push_back(out.str());
}

void Utsi2Utms::addWarning(const std::shared_ptr<SI::Frame> frame, const std::string& message)
{
    std::stringstream out;

    const auto fs = frame->frameSet();

    out << fs->name() << "." << fs->frames().getName(frame).first
        << ": " << message;

    _warnings.push_back(out.str());
}