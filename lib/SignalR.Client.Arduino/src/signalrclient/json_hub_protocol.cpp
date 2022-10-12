// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "json_hub_protocol.h"
#include "message_type.h"
#include "signalrclient/signalr_exception.h"
#include "json_helpers.h"
#include <ArduinoJson.h>

namespace signalr
{
    std::string signalr::json_hub_protocol::write_message(const hub_message* hub_message) const
    {
        // Max message size is 4096 bytes
        DynamicJsonDocument object(4096);

#pragma warning (push)
#pragma warning (disable: 4061)
        switch (hub_message->message_type)
        {
        case message_type::invocation:
        {
            auto invocation = static_cast<invocation_message const*>(hub_message);
            object["type"] = static_cast<int>(invocation->message_type);
            if (!invocation->invocation_id.empty())
            {
                object["invocationId"] = invocation->invocation_id;
            }
            object["target"] = invocation->target;
            auto arguments = object.createNestedArray("arguments");
            createJson(invocation->arguments, arguments, true);
            // TODO: streamIds

            break;
        }
        case message_type::completion:
        {
            auto completion = static_cast<completion_message const*>(hub_message);
            object["type"] = static_cast<int>(completion->message_type);
            object["invocationId"] = completion->invocation_id;
            if (!completion->error.empty())
            {
                object["error"] = completion->error;
            }
            else if (completion->has_result)
            {
                if (completion->result.is_array())
                {
                    auto result = object.createNestedArray("result");
                    createJson(completion->result, result, true);
                }
                else
                {
                    auto v = completion->result;

                    switch (v.type())
                    {
                        case signalr::value_type::boolean:
                        {
                            object["result"] = v.as_bool();
                            break;
                        }
                        case signalr::value_type::float64:
                        {
                            object["result"] = v.as_double();
                            break;
                        }
                        case signalr::value_type::string:
                        {
                            object["result"] = v.as_string();
                            break;
                        }
                        case signalr::value_type::map:
                        {
                            const auto& obj = v.as_map();
                            auto nobj = object.createNestedObject("result");
                            for (auto& val : obj)
                            {
                                // allocate the memory for the document
                                // create an empty array
                                StaticJsonDocument<128> doc;
                                JsonArray array = doc.to<JsonArray>();
                                
                                createJson(val.second, array);

                                nobj[val.first] = array.getElement(0);
                            }
                            break;
                        }
                        case signalr::value_type::null:
                        default:
                        {
                            object["result"] = nullptr;
                            break;
                        }
                    }
                }
            }
            break;
        }
        case message_type::ping:
        {
            auto ping = static_cast<ping_message const*>(hub_message);
            object["type"] = static_cast<int>(ping->message_type);
            break;
        }
        // TODO: other message types
        default:
            break;
        }
#pragma warning (pop)

        std::string output;

        serializeJson(object, output);

        output.push_back(record_separator);

        return output;
    }

    std::vector<std::unique_ptr<hub_message>> json_hub_protocol::parse_messages(const std::string& message) const
    {
        std::vector<std::unique_ptr<hub_message>> vec;
        size_t offset = 0;
        auto pos = message.find(record_separator, offset);
        while (pos != std::string::npos)
        {
            auto hub_message = parse_message(message.c_str() + offset, pos - offset);
            if (hub_message != nullptr)
            {
                vec.push_back(std::move(hub_message));
            }

            offset = pos + 1;
            pos = message.find(record_separator, offset);
        }
        // if offset < message.size()
        // log or close connection because we got an incomplete message
        return vec;
    }

    std::unique_ptr<hub_message> json_hub_protocol::parse_message(const char* begin, size_t length) const
    {
        std::string errors;
        // Max message size is 4096 bytes
        DynamicJsonDocument doc(4096);

        auto er = deserializeJson(doc, begin, length);

        if (er)
        {
            throw signalr_exception(er.c_str());
        }

        if (!doc.is<JsonObject>())
        {
            throw signalr_exception("Message was not a 'map' type");
        }

        const auto& obj = doc.as<JsonObject>();
 
        if (!obj.containsKey("type"))
        {
            throw signalr_exception("Field 'type' not found");
        }

        std::unique_ptr<hub_message> hub_message;

#pragma warning (push)
        // not all cases handled (we have a default so it's fine)
#pragma warning (disable: 4061)
        switch (static_cast<message_type>(obj["type"].as<int>()))
        {
        case message_type::invocation:
        {
            if (!obj.containsKey("target"))
            {
                throw signalr_exception("Field 'target' not found for 'invocation' message");
            }
            if (!obj["target"].is<std::string>())
            {
                throw signalr_exception("Expected 'target' to be of type 'string'");
            }

            if (!obj.containsKey("arguments"))
            {
                throw signalr_exception("Field 'arguments' not found for 'invocation' message");
            }
            if (!obj["arguments"].is<JsonArray>())
            {
                throw signalr_exception("Expected 'arguments' to be of type 'array'");
            }

            std::string invocation_id;
            if (!obj.containsKey("invocationId"))
            {
                invocation_id = "";
            }
            else
            {
                if (!obj["invocationId"].is<std::string>())
                {
                    throw signalr_exception("Expected 'invocationId' to be of type 'string'");
                }
                invocation_id = obj["invocationId"].as<std::string>();
            }

            hub_message = std::unique_ptr<signalr::hub_message>(new invocation_message(invocation_id,
                obj["target"].as<std::string>(), createValue(obj["arguments"].as<JsonArray>()).as_array()));

            break;
        }
        case message_type::completion:
        {
            bool has_result = false;
            signalr::value result;
            if (obj.containsKey("result"))
            {
                has_result = true;
                result = createValue(obj["result"]);
            }

            std::string error;
            if (!obj.containsKey("error"))
            {
                error = "";
            }
            else
            {
                if (obj["error"].is<std::string>())
                {
                    error = obj["error"].as<std::string>();
                }
                else
                {
                    throw signalr_exception("Expected 'error' to be of type 'string'");
                }
            }

            if (!obj.containsKey("invocationId"))
            {
                throw signalr_exception("Field 'invocationId' not found for 'completion' message");
            }
            else
            {
                if (!obj["invocationId"].is<std::string>())
                {
                    throw signalr_exception("Expected 'invocationId' to be of type 'string'");
                }
            }

            if (!error.empty() && has_result)
            {
                throw signalr_exception("The 'error' and 'result' properties are mutually exclusive.");
            }

            hub_message = std::unique_ptr<signalr::hub_message>(new completion_message(obj["invocationId"].as<std::string>(),
                error, result, has_result));

            break;
        }
        case message_type::ping:
        {
            hub_message = std::unique_ptr<signalr::hub_message>(new ping_message());
            break;
        }
        // TODO: other message types
        default:
            // Future protocol changes can add message types, old clients can ignore them
            // TODO: null
            break;
        }
#pragma warning (pop)

        return hub_message;
    }
}