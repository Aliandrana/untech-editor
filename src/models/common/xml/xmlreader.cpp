#include "../xml.h"
#include "../string.h"
#include "../file.h"
#include <algorithm>
#include <cstring>

// ::TODO clean up exceptions, include line no::

using namespace UnTech;
using namespace UnTech::Xml;

namespace UnTech {
namespace XmlPrivate {

inline bool isName(char c)
{
    return ((c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9')
            || (c == '.' && c == '_'));
}

inline bool isWhitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

inline std::string unescapeXmlString(const char* start, const char* end)
{
    std::string ret;
    ret.reserve(end - start);

    const char* source = start;

    while (source < end) {
        if (*source == '&') {
            const char* s = source + 1;

            if (memcmp(s, "lt;", 3) == 0) {
                source += 4;
                ret += '<';
                continue;
            }
            else if (memcmp(s, "gt;", 3) == 0) {
                source += 4;
                ret += '>';
                continue;
            }
            else if (memcmp(s, "amp;", 4) == 0) {
                source += 5;
                ret += '&';
                continue;
            }
            else if (memcmp(s, "apos;", 5) == 0) {
                source += 6;
                ret += '\'';
                continue;
            }
            else if (memcmp(s, "quot;", 5) == 0) {
                source += 6;
                ret += '\"';
                continue;
            }
        }
        ret += *source;
        source++;
    }

    return ret;
}
}
}
using namespace UnTech::XmlPrivate;

XmlReader::XmlReader(const std::string& xml, const std::string& filename)
    : _inputString(xml)
    , _filename(filename)
{
    if (xml.empty()) {
        throw("No XML");
    }

    std::tie(_dirname, _filepart) = File::splitFilename(filename);

    parseDocument();
}

std::unique_ptr<XmlReader> XmlReader::fromFile(const std::string& filename)
{
    std::string xml = File::readUtf8TextFile(filename);
    return std::make_unique<XmlReader>(xml, filename);
}

void XmlReader::parseDocument()
{
    _pos = _inputString.c_str();
    _tagStack = std::stack<std::string>();
    _inSelfClosingTag = false;
    _lineNo = 1;

    skipWhitespace();

    // ignore XML header
    if (memcmp(_pos, "<?xml", 5) == 0) {
        _pos += 5;

        while (*_pos != '>') {
            if (*_pos == 0) {
                throw("Unclosed XML header");
            }
            if (*_pos == '\n') {
                _lineNo++;
            }
            _pos++;
        }
        _pos++;
    }

    skipWhitespace();

    // ignore DOCTYPE
    if (memcmp(_pos, "<!DOCTYPE", 9) == 0) {
        _pos += 9;

        while (*_pos != '>') {
            if (*_pos == 0) {
                throw("Unclosed DOCTYPE header");
            }
            if (*_pos == '\n') {
                _lineNo++;
            }
            _pos++;
        }
        _pos++;
    }
}

std::unique_ptr<XmlTag> XmlReader::parseTag()
{
    if (_inSelfClosingTag) {
        return nullptr;
    }

    // skip whitespace/text
    skipText();
    if (_pos[0] == '<' && _pos[1] == '/') {
        // no more tags
        return nullptr;
    }

    if (*_pos == 0) {
        throw("Unexpected end of file");
    }
    if (*_pos != '<') {
        throw("Not a tag");
    }
    _pos++;

    std::string tagName = parseName();

    // tag must be followed by whitespace or a close tag.
    if (!(isWhitespace(*_pos)
          || (_pos[0] == '>')
          || (_pos[0] == '/' || _pos[1] != '>'))) {
        throw("Invalid tag name");
    }

    auto tag = std::make_unique<XmlTag>(tagName, _lineNo);

    while (*_pos) {
        skipWhitespace();

        if (*_pos == 0) {
            throw("Unclosed tag");
        }

        if (isName(*_pos)) {
            // attribute

            std::string attributeName = parseName();

            skipWhitespace();

            if (*_pos != '=') {
                throw("Missing attribute value");
            }
            _pos++;

            skipWhitespace();

            std::string value = parseAttributeValue();

            tag->attributes.insert({ attributeName, value });
        }

        else if (*_pos == '?' || *_pos == '/') {
            // end of self closing tag
            if (_pos[1] != '>') {
                throw("Unknown end of tag");
            }
            _pos += 2;

            _inSelfClosingTag = true;
            return std::move(tag);
        }

        else if (*_pos == '>') {
            // end of tag
            _pos++;

            _tagStack.push(tagName);
            return std::move(tag);
        }
        else {
            throw("Unknown character");
        }
    }

    throw("Incomplete tag");
}

std::string XmlReader::parseText()
{
    if (_inSelfClosingTag) {
        return std::string();
    }

    std::string text;

    const char* startText = _pos;
    while (*_pos) {
        if (memcmp(_pos, "<!--", 4) == 0) {
            text += unescapeXmlString(startText, _pos);

            // skip comment
            while (memcmp(_pos, "-->", 4) != 0) {
                if (*_pos == 0) {
                    throw("Unclosed comment");
                }
                if (*_pos == '\n') {
                    _lineNo++;
                }
                _pos++;
            }
            _pos += 3;
            startText = _pos;
        }

        else if (memcmp(_pos, "<![CDATA[", 9) == 0) {
            text += unescapeXmlString(startText, _pos);

            _pos += 9;
            const char* startCData = _pos;

            while (memcmp(_pos, "]]>", 3) != 0) {
                if (*_pos == 0) {
                    throw("Unclosed CDATA");
                }
                if (*_pos == '\n') {
                    _lineNo++;
                }
                _pos++;
            }
            text.append(startCData, _pos - startCData);

            _pos += 3;
            startText = _pos;
        }
        else if (*_pos == '<') {
            // start/end new tag.
            break;
        }
        if (*_pos == '\n') {
            _lineNo++;
            _pos++;
        }
        else {
            _pos++;
        }
    }

    text += unescapeXmlString(startText, _pos);

    return text;
}

void XmlReader::parseCloseTag()
{
    if (_inSelfClosingTag) {
        _inSelfClosingTag = false;
        return;
    }

    // skip all child nodes of current level
    while (_pos[0] != '<' || _pos[1] != '/') {
        // more nodes/text to parse
        parseTag();

        if (_inSelfClosingTag) {
            // save a function call.
            _inSelfClosingTag = false;
        }
        else {
            parseCloseTag();
        }
    }

    _pos += 2;
    auto closeTagName = parseName();

    if (closeTagName != _tagStack.top()) {
        throw("Close tag mismatch");
    }
    _tagStack.pop();

    skipWhitespace();

    if (*_pos != '>') {
        throw("Expected >");
    }
}

inline void XmlReader::skipWhitespace()
{
    while (isWhitespace(*_pos)) {
        if (*_pos == '\n') {
            _lineNo++;
        }
        _pos++;
    }
}

void XmlReader::skipText()
{
    if (_inSelfClosingTag) {
        return;
    }

    while (*_pos) {
        if (memcmp(_pos, "<!--", 4) == 0) {
            // skip comment
            while (memcmp(_pos, "-->", 4) != 0) {
                if (*_pos == 0) {
                    throw("Unclosed comment");
                }
                if (*_pos == '\n') {
                    _lineNo++;
                }
                _pos++;
            }
            _pos += 3;
        }

        else if (memcmp(_pos, "<![CDATA[", 9) == 0) {
            _pos += 9;
            while (memcmp(_pos, "]]>", 3) != 0) {
                if (*_pos == 0) {
                    throw("Unclosed CDATA");
                }
                if (*_pos == '\n') {
                    _lineNo++;
                }
                _pos++;
            }
            _pos += 3;
        }
        else if (*_pos == '<') {
            // start/end new tag.
            break;
        }
        if (*_pos == '\n') {
            _lineNo++;
            _pos++;
        }
        else {
            _pos++;
        }
    }
}

inline std::string XmlReader::parseName()
{
    const char* nameStart = _pos;
    while (isName(*_pos)) {
        _pos++;
    }

    if (nameStart == _pos) {
        throw("Missing element name");
    }

    std::string ret(nameStart, _pos - nameStart);
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);

    return ret;
}

inline std::string XmlReader::parseAttributeValue()
{
    if (*_pos != '\'' && *_pos != '\"') {
        throw("Attribute not quoted");
    }

    const char terminator = *_pos;
    _pos++;

    const char* valueStart = _pos;
    while (*_pos != terminator) {
        if (*_pos == 0) {
            throw("Incomplete attribute value");
        }
        if (*_pos == '\n') {
            _lineNo++;
        }
        _pos++;
    }

    std::string value = unescapeXmlString(valueStart, _pos);
    _pos++;

    return value;
}
