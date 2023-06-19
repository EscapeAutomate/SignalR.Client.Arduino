/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Builder for constructing URIs.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "cpprest/uri_builder.h"

static const utility::string_t oneSlash = _XPLATSTR("/");

namespace web
{
uri_builder& uri_builder::append_path(const utility::string_t& toAppend, bool do_encode)
{
    if (!toAppend.empty() && toAppend != oneSlash)
    {
        auto& thisPath = m_uri.m_path;
        if (&thisPath == &toAppend)
        {
            auto appendCopy = toAppend;
            return append_path(appendCopy, do_encode);
        }

        if (thisPath.empty() || thisPath == oneSlash)
        {
            thisPath.clear();
            if (toAppend.front() != _XPLATSTR('/'))
            {
                thisPath.push_back(_XPLATSTR('/'));
            }
        }
        else if (thisPath.back() == _XPLATSTR('/') && toAppend.front() == _XPLATSTR('/'))
        {
            thisPath.pop_back();
        }
        else if (thisPath.back() != _XPLATSTR('/') && toAppend.front() != _XPLATSTR('/'))
        {
            thisPath.push_back(_XPLATSTR('/'));
        }
        else
        {
            // Only one slash.
        }

        if (do_encode)
        {
            thisPath.append(uri::encode_uri(toAppend, uri::components::path));
        }
        else
        {
            thisPath.append(toAppend);
        }
    }

    return *this;
}

uri_builder& uri_builder::append_query(const utility::string_t& toAppend, bool do_encode)
{
    if (!toAppend.empty())
    {
        auto& thisQuery = m_uri.m_query;
        if (&thisQuery == &toAppend)
        {
            auto appendCopy = toAppend;
            return append_query(appendCopy, do_encode);
        }

        if (thisQuery.empty())
        {
            thisQuery.clear();
        }
        else if (thisQuery.back() == _XPLATSTR('&') && toAppend.front() == _XPLATSTR('&'))
        {
            thisQuery.pop_back();
        }
        else if (thisQuery.back() != _XPLATSTR('&') && toAppend.front() != _XPLATSTR('&'))
        {
            thisQuery.push_back(_XPLATSTR('&'));
        }
        else
        {
            // Only one ampersand.
        }

        if (do_encode)
        {
            thisQuery.append(uri::encode_uri(toAppend, uri::components::query));
        }
        else
        {
            thisQuery.append(toAppend);
        }
    }

    return *this;
}

utility::string_t uri_builder::to_string() const { return to_uri().to_string(); }

uri uri_builder::to_uri() const { return uri(m_uri); }

} // namespace web
