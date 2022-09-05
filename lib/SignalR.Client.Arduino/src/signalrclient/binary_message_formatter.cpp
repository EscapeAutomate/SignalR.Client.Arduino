// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <signalrclient/binary_message_formatter.h>
#include <cassert>
#include <signalrclient/signalr_exception.h>

namespace signalr
{
    namespace binary_message_formatter
    {
        void write_length_prefix(std::string& payload)
        {
            // We support payloads up to 2GB so the biggest number we support is 7fffffff which when encoded as
            // VarInt is 0xFF 0xFF 0xFF 0xFF 0x07 - hence the maximum length prefix is 5 bytes.
            if (payload.length() > 4096)
            {
                throw signalr_exception("messages over 4Ko are not supported.");
            }

            char buffer[5];

            size_t length = payload.length();
            size_t length_num_bytes = 0;
            do
            {
                buffer[length_num_bytes] = (char)(length & 0x7f);
                length >>= 7;
                if (length > 0)
                {
                    buffer[length_num_bytes] |= 0x80;
                }
                length_num_bytes++;
            } while (length > 0 && length_num_bytes < 5);

            payload.insert(0, buffer, length_num_bytes);
        }
    }
}
