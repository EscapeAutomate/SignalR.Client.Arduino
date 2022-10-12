// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "json_helpers.h"
#include <cmath>
#include <stdint.h>
#include <ArduinoJson.h>

namespace signalr
{
    char record_separator = '\x1e';
    
    signalr::value createValue(const JsonVariant& v)
    {        
        if(v.is<bool>())
        {
            return signalr::value(v.as<bool>());
        }
        else if(v.is<double>())
        {
            return signalr::value(v.as<double>());
        }
        else if(v.is<std::string>())
        {
            return signalr::value(v.as<std::string>());
        }
        else if(v.is<JsonArray>())
        {
            auto arr = v.as<JsonArray>();
            std::vector<signalr::value> vec;
            for (JsonVariant ar : arr) {
                vec.push_back(createValue(ar));
            }
            return signalr::value(std::move(vec));
        }
        else if(v.is<JsonObject>())
        {
            auto obj = v.as<JsonObject>();
            std::map<std::string, signalr::value> map;
            for (JsonPair p : obj) {
                map.insert({ std::string(p.key().c_str()), createValue(p.value().as<JsonVariant>()) });
            }
            return signalr::value(std::move(map));
        }
        
        // if(v.isNull())
        return signalr::value();
    }

    char getBase64Value(uint32_t i)
    {
        char index = (char)i;
        if (index < 26)
        {
            return 'A' + index;
        }
        if (index < 52)
        {
            return 'a' + (index - 26);
        }
        if (index < 62)
        {
            return '0' + (index - 52);
        }
        if (index == 62)
        {
            return '+';
        }
        if (index == 63)
        {
            return '/';
        }

        throw std::out_of_range("base64 table index out of range: " + std::to_string(index));
    }

    std::string base64Encode(const std::vector<uint8_t>& data)
    {
        std::string base64result;

        size_t i = 0;
        while (i <= data.size() - 3)
        {
            uint32_t b = ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8) | (uint32_t)data[i + 2];
            base64result.push_back(getBase64Value((b >> 18) & 0x3F));
            base64result.push_back(getBase64Value((b >> 12) & 0x3F));
            base64result.push_back(getBase64Value((b >> 6) & 0x3F));
            base64result.push_back(getBase64Value(b & 0x3F));

            i += 3;
        }
        if (data.size() - i == 2)
        {
            uint32_t b = ((uint32_t)data[i] << 8) | (uint32_t)data[i + 1];
            base64result.push_back(getBase64Value((b >> 10) & 0x3F));
            base64result.push_back(getBase64Value((b >> 4) & 0x3F));
            base64result.push_back(getBase64Value((b << 2) & 0x3F));
            base64result.push_back('=');
        }
        else if (data.size() - i == 1)
        {
            uint32_t b = (uint32_t)data[i];
            base64result.push_back(getBase64Value((b >> 2) & 0x3F));
            base64result.push_back(getBase64Value((b << 4) & 0x3F));
            base64result.push_back('=');
            base64result.push_back('=');
        }

        return base64result;
    }

    void createJson(const signalr::value& v, JsonArray& packer, bool firstLevel)
    {
        switch (v.type())
        {
            case signalr::value_type::boolean:
            {
                if (v.as_bool())
                {
                    packer.add(true);
                }
                else
                {
                    packer.add(false);
                }
                return;
            }
            case signalr::value_type::float64:
            {
                auto value = v.as_double();
                double intPart;
                // Workaround for 1.0 being output as 1.0 instead of 1
                // because the server expects certain values to be 1 instead of 1.0 (like protocol version)
                if (std::modf(value, &intPart) == 0)
                {
                    if (value < 0)
                    {
                        if (value >= (double)INT64_MIN)
                        {
                            // Fits within int64_t
                            packer.add(static_cast<int64_t>(intPart));
                            return;
                        }
                        else
                        {
                            // Remain as double
                            packer.add(value);
                            return;
                        }
                    }
                    else
                    {
                        if (value <= (double)UINT64_MAX)
                        {
                            // Fits within uint64_t
                            packer.add(static_cast<uint64_t>(intPart));
                            return;
                        }
                        else
                        {
                            // Remain as double
                            packer.add(value);
                            return;
                        }
                    }
                }
                packer.add(v.as_double());
                return;
            }
            case signalr::value_type::string:
            {
                packer.add(v.as_string());
                return;
            }
            case signalr::value_type::array:
            {
                const auto& array = v.as_array();
                
                if (firstLevel)
                {
                    for (auto& val : array)
                    {
                        createJson(val, packer);
                    }
                }
                else  
                {
                    auto arr = packer.createNestedArray();
                    for (auto& val : array)
                    {
                        createJson(val, arr);
                    }
                }                
                return;
            }
            case signalr::value_type::map:
            {
                const auto& obj = v.as_map();
                auto nobj = packer.createNestedObject();
                for (auto& val : obj)
                {
                    // allocate the memory for the document
                    // create an empty array
                    StaticJsonDocument<128> doc;
                    JsonArray array = doc.to<JsonArray>();
                    
                    createJson(val.second, array);

                    nobj[val.first] = array.getElement(0);
                }
                return;
            }
            case signalr::value_type::null:
            default:
            {
                packer.add(nullptr);
                return;
            }
        }
    }
}