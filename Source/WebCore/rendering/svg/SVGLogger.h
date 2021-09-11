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

#pragma once

#include "Logging.h"
#include <wtf/text/StringConcatenate.h>
#include <wtf/text/TextStream.h>

namespace WebCore {

// Helpers for pretty printing
String repeatedString(const String&, unsigned repetitions);
String leftPaddedString(const String&, unsigned padding);
String rightPaddedString(const String&, unsigned padding);

class SVGLogger {
    WTF_MAKE_NONCOPYABLE(SVGLogger);

public:
    using LogFunction = std::function<void(TextStream&)>;

private:
    template<WTFLogLevel logLevel>
    struct Scope {
        Scope(LogFunction beginLogFunction, LogFunction _endLogFunction, std::function<void()> _exitFunction)
            : endLogFunction(_endLogFunction)
            , exitFunction(_exitFunction)
        {
            SVGLogger logger(LOG_CHANNEL(SVG), logLevel);
            beginLogFunction(logger.prefixedStream());

            ++SVGLogger::loggerNestingDepth();
        }

        ~Scope()
        {
            --SVGLogger::loggerNestingDepth();

            {
                SVGLogger logger(LOG_CHANNEL(SVG), logLevel);
                endLogFunction(logger.prefixedStream());
            }

            if (exitFunction)
                exitFunction();
        }

        LogFunction endLogFunction;
        std::function<void()> exitFunction;
    };

public:
    struct InfoScope : Scope<WTFLogLevel::Info> {
        InfoScope(LogFunction _beginLogFunction, LogFunction _endLogFunction = { }, std::function<void()> _exitFunction = { })
            : Scope(_beginLogFunction, _endLogFunction, _exitFunction) { }
    };

    struct ErrorScope : Scope<WTFLogLevel::Error> {
        ErrorScope(LogFunction _beginLogFunction, LogFunction _endLogFunction = { }, std::function<void()> _exitFunction = { })
            : Scope(_beginLogFunction, _endLogFunction, _exitFunction) { }
    };

    struct WarningScope : Scope<WTFLogLevel::Warning> {
        WarningScope(LogFunction _beginLogFunction, LogFunction _endLogFunction = { }, std::function<void()> _exitFunction = { })
            : Scope(_beginLogFunction, _endLogFunction, _exitFunction) { }
    };

    struct DebugScope : Scope<WTFLogLevel::Debug> {
        DebugScope(LogFunction _beginLogFunction, LogFunction _endLogFunction = { }, std::function<void()> _exitFunction = { })
            : Scope(_beginLogFunction, _endLogFunction, _exitFunction) { }
    };

#if LOG_DISABLED || defined(NDEBUG)
    static void info(LogFunction) { }
    static void error(LogFunction) { }
    static void warning(LogFunction) { }
    static void debug(LogFunction) { }
#else
    static void info(LogFunction logFunction)
    {
        SVGLogger logger(LOG_CHANNEL(SVG), WTFLogLevel::Info);
        logFunction(logger.prefixedStream());
    }

    static void error(LogFunction logFunction)
    {
        SVGLogger logger(LOG_CHANNEL(SVG), WTFLogLevel::Error);
        logFunction(logger.prefixedStream());
    }

    static void warning(LogFunction logFunction)
    {
        SVGLogger logger(LOG_CHANNEL(SVG), WTFLogLevel::Warning);
        logFunction(logger.prefixedStream());
    }

    static void debug(LogFunction logFunction)
    {
        SVGLogger logger(LOG_CHANNEL(SVG), WTFLogLevel::Debug);
        logFunction(logger.prefixedStream());
    }
#endif

protected:
    SVGLogger(WTFLogChannel&, WTFLogLevel);
    ~SVGLogger();

    TextStream& prefixedStream() { return m_stream << accumulatedLinePrefix(); }

    // Each log item is a box, and can have multiple sections with information.
    enum class BoxOptions : uint8_t {
        HasChildren    = 1 << 0,
        HasParent      = 1 << 1,
        HasNextSibling = 1 << 2
    };
    void addBox(const String& boxTitle, const String& boxInformation, const OptionSet<BoxOptions>& options = { });

    void addText(const String&);

    struct SectionScope {
        SectionScope(SVGLogger& _logger, const String& title)
            : logger(_logger)
        {
            logger.addNewLineAndPrefix();
            logger.pushLinePrefix(logger.sectionIdentationString());
            logger.addSectionTitle(title);
        }

        ~SectionScope()
        {
            logger.popLinePrefix();
        }

        void addEntry(const String& entry)
        {
            logger.addSectionEntry(entry);
        }

        template<typename T>
        void addEntry(const String& name, const T& object, unsigned namePadding)
        {
            logger.addSectionEntry(name, object, namePadding);
        }

        void addNewLine()
        {
            logger.addNewLineAndPrefix();
        }

        SVGLogger& logger;
    };

    void addNewLineAndPrefix();
    void addNewLine();

    void pushLinePrefix(const String& linePrefix) { m_linePrefixes.append(linePrefix); }
    void popLinePrefix() { m_linePrefixes.removeLast(); }

    bool loggingDisabled() const { return m_logChannel.state == WTFLogChannelState::Off; }

    static unsigned& loggerNestingDepth();

private:
    void addSectionTitle(const String&);
    void addSectionEntry(const String&);

    template<typename T>
    void addSectionEntry(const String& name, const T& object, unsigned namePadding)
    {
        TextStream stream(TextStream::LineMode::SingleLine);

        auto description = leftPaddedString(makeString(name, "="), namePadding);

        TextStream objectStream(TextStream::LineMode::SingleLine);
        objectStream << object;

        auto objectAsString = objectStream.release();

        // e.g. TransformationMatrix dumps with an initial newline -- remove it for our purposes.
        if (objectAsString.startsWith('\n'))
            objectAsString.remove(0);

        // Fix up formatting for multi-line strings.
        objectAsString.replace('\n', makeString('\n', accumulatedLinePrefix(), repeatedString(" ", description.length() + 2)));

        addSectionEntry(makeString(description, objectAsString));
    }

    String accumulatedLinePrefix() const;
    const String& sectionIdentationString() const { return m_sectionIndentationString; }

private:
    Vector<String> m_linePrefixes;

    TextStream m_stream { TextStream::LineMode::SingleLine };
    String m_sectionIndentationString;

    WTFLogChannel& m_logChannel;
    WTFLogLevel m_logLevel { WTFLogLevel::Debug };
};

} // namespace WebCore
