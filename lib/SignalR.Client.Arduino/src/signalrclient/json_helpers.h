// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "signalrclient/signalr_value.h"
#include <memory>
#include <ArduinoJson.h>

namespace signalr
{
    static constexpr char record_separator = '\x1e';

    signalr::value createValue(const JsonVariant& v);

    void createJson(const signalr::value& v, JsonArray& packer, bool firstLevel = false);

    std::string base64Encode(const std::vector<uint8_t>& data);
}