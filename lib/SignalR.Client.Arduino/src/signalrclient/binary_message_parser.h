// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#ifdef USE_MSGPACK

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ long unsigned int
typedef __SIZE_TYPE__ size_t;
#endif

namespace signalr
{
    namespace binary_message_parser
    {
        bool try_parse_message(const unsigned char* message, size_t length, size_t* length_prefix_length, size_t* length_of_message);
    }
}

#endif