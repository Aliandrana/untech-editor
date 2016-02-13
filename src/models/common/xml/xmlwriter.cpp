#include "xmlwriter.h"
#include "../string.h"
#include "../file.h"
#include <algorithm>
#include <cstring>
#include <cassert>
#include <sstream>

using namespace UnTech;
using namespace UnTech::Xml;

XmlWriter::XmlWriter(std::ostream& output, const std::string& doctype)
    : _file(output)
    , _tagStack()
{
    _file << "<?xml version=\"1.0\" encoding=\"UTF_8\"?>\n";

    if (!doctype.empty()) {
        assert(isName(doctype));
        _file << "<!DOCTYPE " << doctype << ">\n";
    }

    _inTag = false;
}

XmlWriter::~XmlWriter()
{
    assert(_tagStack.size() == 0);
}

void XmlWriter::writeTag(const std::string& name)
{
    assert(isName(name));

    if (_inTag) {
        writeCloseTagHead();
    }

    // Indent
    for (size_t i = 0; i < _tagStack.size(); i++) {
        _file << "  ";
    }

    _file << "<" << name;

    _tagStack.push(name);
    _inTag = true;
}

void XmlWriter::writeTagAttribute(const std::string& name, const std::string& value)
{
    assert(_inTag);
    assert(isName(name));

    _file << ' ' << name << "=\"";
    writeEscapeAttribute(value);
    _file << '"';
}

void XmlWriter::writeTagAttribute(const std::string& name, const char* value)
{
    assert(_inTag);
    assert(isName(name));

    _file << ' ' << name << "=\"";
    writeEscapeAttribute(value);
    _file << '"';
}

void XmlWriter::writeTagAttribute(const std::string& name, const int value)
{
    assert(_inTag);
    assert(isName(name));
    assert(_file.width() == 0);
    assert((_file.flags() & std::ios_base::basefield) == std::ios_base::dec);

    _file << ' ' << name << "=\"" << value << '"';
}

void XmlWriter::writeTagAttribute(const std::string& name, const unsigned value)
{
    assert(_inTag);
    assert(isName(name));
    assert(_file.width() == 0);
    assert((_file.flags() & std::ios_base::basefield) == std::ios_base::dec);

    _file << ' ' << name << "=\"" << value << '"';
}

void XmlWriter::writeText(const std::string& text)
{
    if (_inTag) {
        writeCloseTagHead();
    }

    for (char c : text) {
        switch (c) {

        case '&':
            _file << "&amp;";
            break;

        case '<':
            _file << "&lt;";
            break;

        case '>':
            _file << "&gt;";
            break;

        default:
            _file << c;
        }
    }
}

void XmlWriter::writeCloseTag()
{
    if (_inTag) {
        _file << "/>\n";
    }
    else {
        assert(_tagStack.size() > 0);

        for (size_t i = 1; i < _tagStack.size(); i++) {
            _file << "  ";
        }

        _file << "</" << _tagStack.top() << ">\n";
    }

    _inTag = false;
    _tagStack.pop();
}

inline void XmlWriter::writeCloseTagHead()
{
    assert(_inTag);

    _file << ">\n";

    _inTag = false;
}

inline void XmlWriter::writeEscapeAttribute(const std::string& text)
{
    for (char c : text) {
        switch (c) {

        case '&':
            _file << "&amp;";
            break;

        case '<':
            _file << "&lt;";
            break;

        case '>':
            _file << "&gt;";
            break;

        case '"':
            _file << "&quot;";
            break;

        default:
            _file << c;
        }
    }
}

inline void XmlWriter::writeEscapeAttribute(const char* text)
{
    const char* pos = text;
    while (*pos) {
        switch (*pos) {

        case '&':
            _file << "&amp;";
            break;

        case '<':
            _file << "&lt;";
            break;

        case '>':
            _file << "&gt;";
            break;

        case '"':
            _file << "&quot;";
            break;

        default:
            _file << *pos;
        }

        pos++;
    }
}