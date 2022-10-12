// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "handshake_protocol.h"
#include "json_helpers.h"
#include "signalrclient/signalr_exception.h"

namespace signalr
{
    namespace handshake
    {
        std::string write_handshake(hub_protocol* protocol)
        {
            auto map = std::map<std::string, signalr::value>
            {
                { "protocol", signalr::value(protocol->name()) },
                { "version", signalr::value((double)protocol->version()) }
            };

            ArduinoJson::StaticJsonDocument<60> doc;
            JsonArray array = doc.to<JsonArray>();

            createJson(signalr::value(std::move(map)), array);

            std::string serialized;

            serializeJson(array, serialized);

            return serialized + record_separator;
        }

        std::tuple<std::string, signalr::value> parse_handshake(const std::string& response)
        {
            auto pos = response.find(record_separator);
            if (pos == std::string::npos)
            {
                throw signalr_exception("incomplete message received");
            }
            auto message = response.substr(0, pos);

            ArduinoJson::StaticJsonDocument<400> doc;

            if (ArduinoJson::deserializeJson(doc, message.c_str(), message.size()) != DeserializationError::Code::Ok)
            {
                throw signalr_exception("deserialization error");
            }
            auto remaining_data = response.substr(pos + 1);
            return std::forward_as_tuple(remaining_data, createValue(doc));
        }
    }
}