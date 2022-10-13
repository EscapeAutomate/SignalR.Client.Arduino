// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "logger.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <assert.h>

namespace signalr
{
    logger::logger(log_writer* log_writer, trace_level trace_level) noexcept
        : m_trace_level(trace_level)
    { 
        m_log_writer = log_writer;
    }

    void logger::log(trace_level level, const std::string& entry) const
    {
        log(level, entry.data(), entry.length());
    }

    void logger::log(trace_level level, const char* entry, size_t entry_count) const
    {
        if (level >= m_trace_level && m_log_writer)
        {
            try
            {
                std::stringstream os;

                write_trace_level(level, os);

                os.write(entry, (std::streamsize)entry_count) << std::endl;
                m_log_writer->write(os.str());
            }
            catch (const std::exception &e)
            {
                std::cerr << "error occurred when logging: " << e.what()
                    << std::endl << "    entry: " << entry << std::endl;
            }
            catch (...)
            {
                std::cerr << "unknown error occurred when logging" << std::endl << "    entry: " << entry << std::endl;
            }
        }
    }

    void logger::write_trace_level(trace_level trace_level, std::ostream& stream)
    {
        switch (trace_level)
        {
        case signalr::trace_level::verbose:
            stream << " [verbose  ] ";
            break;
        case signalr::trace_level::debug:
            stream << " [debug    ] ";
            break;
        case signalr::trace_level::info:
            stream << " [info     ] ";
            break;
        case signalr::trace_level::warning:
            stream << " [warning  ] ";
            break;
        case signalr::trace_level::error:
            stream << " [error    ] ";
            break;
        case signalr::trace_level::critical:
            stream << " [critical ] ";
            break;
        case signalr::trace_level::none:
            stream << " [none     ] ";
            break;
        default:
            assert(false);
            stream << " [(unknown)] ";
            break;
        }
    }
}
