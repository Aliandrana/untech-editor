#include "string.h"
#include <string>
#include <fstream>

using namespace UnTech;

bool String::checkUtf8WellFormed(const std::string& str)
{
    const unsigned char* c = (const unsigned char*)str.c_str();
    size_t length = 0;

    if (str.empty()) {
        return true;
    }

    while (*c) {
        // Code based off The Unicode 7.0 Standard
        // Table 3-7. *Well-Formed UTF-8 Byte Sequences*

        if (c[0] <= 0x7F) {
            c++;
            length++;
        }
        else if ((c[0] >= 0xC2 && c[0] <= 0xDF)
                 && (c[1] >= 0x80 && c[1] <= 0xBF)) {
            c += 2;
            length += 2;
        }
        else if (c[0] == 0xE0
                 && (c[1] >= 0xA0 && c[1] <= 0xBF)
                 && (c[2] >= 0x80 && c[2] <= 0xBF)) {
            c += 3;
            length += 3;
        }
        else if ((c[0] == 0xE1 && c[0] <= 0xEC)
                 && (c[1] >= 0x80 && c[1] <= 0xBF)
                 && (c[2] >= 0x80 && c[2] <= 0xBF)) {
            c += 3;
            length += 3;
        }
        else if (c[0] == 0xED
                 && (c[1] >= 0x80 && c[1] <= 0x9F)
                 && (c[2] >= 0x80 && c[2] <= 0xBF)) {
            c += 3;
            length += 3;
        }
        else if ((c[0] >= 0xEE && c[0] <= 0xEF)
                 && (c[1] >= 0x80 && c[1] <= 0xBF)
                 && (c[2] >= 0x80 && c[2] <= 0xBF)) {
            c += 3;
            length += 3;
        }
        else if (c[0] == 0xF0
                 && (c[1] >= 0x90 && c[1] <= 0xBF)
                 && (c[2] >= 0x80 && c[2] <= 0xBF)
                 && (c[3] >= 0x80 && c[3] <= 0xBF)) {
            c += 4;
            length += 4;
        }
        else if ((c[0] >= 0xF1 && c[0] <= 0xF3)
                 && (c[1] >= 0x80 && c[1] <= 0xBF)
                 && (c[2] >= 0x80 && c[2] <= 0xBF)
                 && (c[3] >= 0x80 && c[3] <= 0xBF)) {
            c += 4;
            length += 4;
        }
        else if (c[0] == 0xF4
                 && (c[1] >= 0x80 && c[1] <= 0x8F)
                 && (c[2] >= 0x80 && c[2] <= 0xBF)
                 && (c[3] >= 0x80 && c[3] <= 0xBF)) {
            c += 4;
            length += 4;
        }
        else {
            return false;
        }
    }

    return length == str.size();
}
