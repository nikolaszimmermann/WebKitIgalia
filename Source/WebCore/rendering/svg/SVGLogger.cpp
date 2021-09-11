/**
 * Copyright (C) 2021 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SVGLogger.h"

#include <wtf/Compiler.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

constexpr unsigned sectionIndentationSpaces = 4;
constexpr unsigned nestingIndentationSpaces = 2;

namespace Character {

constexpr auto useUnicodeCharacters = true;
inline String topLeftBoxHeavy()         { return useUnicodeCharacters ? String::fromUTF8("┏") : "+"; }
inline String topLeftBoxLight()         { return useUnicodeCharacters ? String::fromUTF8("┌") : "+"; }
inline String topRightBoxHeavy()        { return useUnicodeCharacters ? String::fromUTF8("┓") : "+"; }
inline String bottomLeftBoxHeavy()      { return useUnicodeCharacters ? String::fromUTF8("┗") : "+"; }
inline String bottomRightBoxHeavy()     { return useUnicodeCharacters ? String::fromUTF8("┛") : "+"; }
inline String verticalLineHeavy()       { return useUnicodeCharacters ? String::fromUTF8("┃") : "|"; }
inline String verticalLineLight()       { return useUnicodeCharacters ? String::fromUTF8("│") : "|"; }
inline String verticalLineLightDouble() { return useUnicodeCharacters ? String::fromUTF8("║") : "|"; }
inline String horizontalLineHeavy()     { return useUnicodeCharacters ? String::fromUTF8("━") : "-"; }
inline String leftBoxConnector()        { return useUnicodeCharacters ? String::fromUTF8("┨") : "|"; }
inline String bottomBoxConnector()      { return useUnicodeCharacters ? String::fromUTF8("┯") : "-"; }
inline String childBoxConnector()       { return useUnicodeCharacters ? String::fromUTF8("├") : "|"; }
inline String lastChildBoxConnector()   { return useUnicodeCharacters ? String::fromUTF8("└") : "|"; }
inline String arrowConnector()          { return useUnicodeCharacters ? String::fromUTF8("⮕") : ">"; }

};

SVGLogger::SVGLogger(WTFLogChannel& logChannel, WTFLogLevel logLevel)
    : m_logChannel(logChannel)
    , m_logLevel(logLevel)
{
    pushLinePrefix(makeString(Character::verticalLineLightDouble(), repeatedString(" ", nestingIndentationSpaces * loggerNestingDepth() + 1)));
}

SVGLogger::~SVGLogger()
{
    WTFLogWithLevel(&m_logChannel, m_logLevel, "%s", m_stream.release().utf8().data());

    popLinePrefix();
    ASSERT(m_linePrefixes.isEmpty());
}

unsigned& SVGLogger::loggerNestingDepth()
{
    static NeverDestroyed<unsigned> depth;
    return depth;
}

String SVGLogger::accumulatedLinePrefix() const
{
    StringBuilder builder;
    for (const auto& linePrefix : m_linePrefixes) {
        if (!linePrefix.isEmpty())
            builder.append(linePrefix);
    }
    return builder.toString();
}

void SVGLogger::addBox(const String& boxTitle, const String& boxInformation, const OptionSet<BoxOptions>& options)
{
    auto horizontalBorder = repeatedString(Character::horizontalLineHeavy(), boxTitle.length());

    bool hasParent = options.contains(BoxOptions::HasParent);
    auto leftConnector = hasParent ? Character::leftBoxConnector : Character::verticalLineHeavy;

    bool hasChildren = options.contains(BoxOptions::HasChildren);
    auto childBoxConnectorFromParent = hasChildren ? Character::bottomBoxConnector : Character::horizontalLineHeavy;

    // Box top line
    prefixedStream() << Character::topLeftBoxHeavy() << Character::horizontalLineHeavy() << Character::horizontalLineHeavy() << horizontalBorder << Character::topRightBoxHeavy();

    // Box middle line
    String middleLinePrefix = accumulatedLinePrefix();
    if (hasParent) {
        ASSERT(middleLinePrefix.length() > 1);

        if (options.contains(BoxOptions::HasNextSibling))
            middleLinePrefix = makeString(middleLinePrefix.substring(0, middleLinePrefix.length() - 1), Character::childBoxConnector());
        else {
            middleLinePrefix = makeString(middleLinePrefix.substring(0, middleLinePrefix.length() - 1), Character::lastChildBoxConnector());

            // The accumulated line prefix already contains a vertical line -- remove it, as
            // we're past the last child box, and no longer need the line.
            popLinePrefix();
            pushLinePrefix("   ");
        }
    }

    addNewLine();
    m_stream << middleLinePrefix << leftConnector() << " " << boxTitle << " " << Character::verticalLineHeavy() << " " << Character::arrowConnector() << " " << boxInformation;

    // Box bottom line
    addNewLine();
    prefixedStream() << Character::bottomLeftBoxHeavy() << Character::horizontalLineHeavy() << childBoxConnectorFromParent() << horizontalBorder << Character::bottomRightBoxHeavy();

    auto indentation = sectionIndentationSpaces;
    if (hasChildren)
        indentation -= 3; // " | "
    m_sectionIndentationString = repeatedString(" ", indentation);

    if (hasChildren)
        pushLinePrefix(makeString("  ", Character::verticalLineLight()));
    else
        pushLinePrefix(String());
}

void SVGLogger::addText(const String& text)
{
    m_stream << text;
}

void SVGLogger::addSectionTitle(const String& sectionTitle)
{
    addNewLineAndPrefix();
    m_stream << Character::arrowConnector() << " " << sectionTitle;
}

void SVGLogger::addSectionEntry(const String& entry)
{
    addNewLineAndPrefix();
    m_stream << "  " << entry;
}

void SVGLogger::addNewLineAndPrefix()
{
    m_stream << "\n" << accumulatedLinePrefix();
}

void SVGLogger::addNewLine()
{
    m_stream << "\n";
}

String repeatedString(const String& string, unsigned repetitions)
{
    StringBuilder stringBuilder;
    stringBuilder.reserveCapacity(repetitions);

    for (unsigned i = 0; i < repetitions; ++i)
        stringBuilder.append(string);

    return stringBuilder.toString();
}

String leftPaddedString(const String& string, unsigned padding)
{
    if (padding <= string.length())
        return string;
    return makeString(repeatedString(" ", padding - string.length()), string);
}

String rightPaddedString(const String& string, unsigned padding)
{
    if (padding <= string.length())
        return string;
    return makeString(string, repeatedString(" ", padding - string.length()));
}

}
