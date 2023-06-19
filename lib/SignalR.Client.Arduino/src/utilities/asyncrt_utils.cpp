/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Utilities
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "cpprest/uri_builder.h"

#include <algorithm>
#include <cpprest/asyncrt_utils.h>
#include <stdexcept>
#include <string>
#include <time.h>

using namespace web;
using namespace utility;
using namespace utility::conversions;

namespace
{
struct to_lower_ch_impl
{
    char operator()(char c) const CPPREST_NOEXCEPT
    {
        if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');
        return c;
    }

    wchar_t operator()(wchar_t c) const CPPREST_NOEXCEPT
    {
        if (c >= L'A' && c <= L'Z') return static_cast<wchar_t>(c - L'A' + L'a');
        return c;
    }
};

CPPREST_CONSTEXPR to_lower_ch_impl to_lower_ch {};

struct eq_lower_ch_impl
{
    template<class CharT>
    inline CharT operator()(const CharT left, const CharT right) const CPPREST_NOEXCEPT
    {
        return to_lower_ch(left) == to_lower_ch(right);
    }
};

CPPREST_CONSTEXPR eq_lower_ch_impl eq_lower_ch {};

struct lt_lower_ch_impl
{
    template<class CharT>
    inline CharT operator()(const CharT left, const CharT right) const CPPREST_NOEXCEPT
    {
        return to_lower_ch(left) < to_lower_ch(right);
    }
};

CPPREST_CONSTEXPR lt_lower_ch_impl lt_lower_ch {};
} // namespace

namespace utility
{
namespace details
{

_ASYNCRTIMP void __cdecl inplace_tolower(std::string& target) CPPREST_NOEXCEPT
{
    for (auto& ch : target)
    {
        ch = to_lower_ch(ch);
    }
}

} // namespace details

} // namespace utility
