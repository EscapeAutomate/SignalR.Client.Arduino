#include "messagepack_hub_protocol.h"
#include "message_type.h"
#include "signalr_exception.h"
#include "binary_message_parser.h"
#include "binary_message_formatter.h"
#include <ArduinoJson.h>
#include <ArduinoJson/MsgPack/endianess.hpp>
#include <cmath>
#include <cassert>

namespace signalr
{
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
        
        return signalr::value();
    }

    void pack_messagepack(const signalr::value& v, JsonArray& packer)
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
            auto arr = packer.createNestedArray();            
            for (auto& val : array)
            {
                pack_messagepack(val, arr);
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
                
                pack_messagepack(val.second, array);

                nobj[val.first] = array.getElement(0);
            }
            return;
        }
        case signalr::value_type::binary:
        {
            size_t l = v.as_binary().size();
            std::vector<uint8_t> bin = v.as_binary();
            uint8_t buffer[5];

            if(l < 256) {
                buffer[0] = 0xc4;
                buffer[1] = static_cast<uint8_t>(l);
                bin.insert(bin.begin(), buffer, buffer + 2);
            } else if(l < 65536) {
                buffer[0] = 0xc5;
                auto val = uint16_t(l);
                ARDUINOJSON_NAMESPACE::fixEndianess(val);
                auto b = reinterpret_cast<uint8_t*>(&val);
                bin.insert(bin.begin(), buffer, buffer + 3);
            } else {
                buffer[0] = 0xc6;
                auto val = uint32_t(l);
                ARDUINOJSON_NAMESPACE::fixEndianess(val);
                auto b = reinterpret_cast<uint8_t*>(&val);
                bin.insert(bin.begin(), buffer, buffer + 5);
            }
            
            auto value = serialized(reinterpret_cast<char*>(bin.data()), bin.size());
            packer.add(value);            
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

    std::string messagepack_hub_protocol::write_message(const hub_message* hub_message) const
    {
        // Max message size is 4096 bytes
        DynamicJsonDocument doc(4096);

        switch (hub_message->message_type)
        {
        case message_type::invocation:
        {
            auto invocation = static_cast<invocation_message const*>(hub_message);

            // MessageType
            doc.add(static_cast<int>(message_type::invocation));

            // Headers
            doc.createNestedObject();

            // InvocationId
            if (invocation->invocation_id.empty())
            {
                doc.add(nullptr);
            }
            else
            {
                doc.add(invocation->invocation_id);
            }
            
            // Target
            doc.add(invocation->target);
            
            // Even if it's called JsonArray it can be serialized in msgpack
            JsonArray arguments = doc.createNestedArray();
            for (auto& val : invocation->arguments)
            {
                pack_messagepack(val, arguments);
            }

            // StreamIds
            doc.createNestedArray();

            break;
        }
        case message_type::completion:
        {
            auto completion = static_cast<completion_message const*>(hub_message);
            
            // MessageType
            doc.add(static_cast<int>(message_type::completion));

            // Headers
            doc.createNestedObject();

            doc.add(completion->invocation_id);

            auto result_kind = completion->error.empty() ? (completion->has_result ? 3U : 2U) : 1U;

            doc.add(result_kind);

            switch (result_kind)
            {
                // error result
            case 1:
                doc.add(completion->error);
                break;
                // non-void result
            case 3:
                auto arr = doc.as<JsonArray>();
                pack_messagepack(completion->result, arr);
                break;
            }

            break;
        }
        case message_type::ping:
        {
            // If we need the ping this is how you get it
            // auto ping = static_cast<ping_message const*>(hub_message);

            doc.add(static_cast<int>(message_type::ping));
            break;
        }
        default:
            break;
        }

         std::string output;
         serializeMsgPack(doc, output);

        binary_message_formatter::write_length_prefix(output);
        return output;
    }

    std::vector<std::unique_ptr<hub_message>> messagepack_hub_protocol::parse_messages(const std::string& message) const
    {
        std::vector<std::unique_ptr<hub_message>> vec;

        size_t length_prefix_length;
        size_t length_of_message;
        const char* remaining_message = message.data();
        size_t remaining_message_length = message.length();

        while (binary_message_parser::try_parse_message(reinterpret_cast<const unsigned char*>(remaining_message), remaining_message_length, &length_prefix_length, &length_of_message))
        {
            assert(length_prefix_length <= remaining_message_length);
            remaining_message += length_prefix_length;
            remaining_message_length -= length_prefix_length;
            assert(remaining_message_length >= length_of_message);
            
            DynamicJsonDocument doc(4096);
            auto err = deserializeMsgPack(doc, remaining_message);

            if (err != DeserializationError::Ok || remaining_message_length < 2)
            {
                throw signalr_exception("messagepack object was incomplete");
            }

            if (!doc.is<JsonArray>())
            {
                throw signalr_exception("Message was not an 'array' type");
            }
            
            JsonArray arr = doc.as<JsonArray>();

            if (arr.size() == 0)
            {
                throw signalr_exception("Message was an empty array");
            }

            if (!arr[0].is<int>())
            {
                throw signalr_exception("reading 'type' as int failed");
            }

            auto msgpack_obj_index = arr.begin();

            auto type = (*msgpack_obj_index).as<int>();
            ++msgpack_obj_index;

            switch (static_cast<message_type>(type))
            {
            case message_type::invocation:
            {
                if (arr.size() < 5)
                {
                    throw signalr_exception("invocation message has too few properties");
                }

                // HEADERS
                ++msgpack_obj_index;

                std::string invocation_id;
                if ((*msgpack_obj_index).is<std::string>())
                {
                    invocation_id.append((*msgpack_obj_index).as<std::string>());
                }
                else if (!(*msgpack_obj_index).isNull())
                {
                    throw signalr_exception("reading 'invocationId' as string failed");
                }
                ++msgpack_obj_index;

                if (!(*msgpack_obj_index).is<std::string>())
                {
                    throw signalr_exception("reading 'target' as string failed");
                }

                std::string target((*msgpack_obj_index).as<std::string>());
                ++msgpack_obj_index;

                if (!(*msgpack_obj_index).is<JsonArray>())
                {
                    throw signalr_exception("reading 'arguments' as array failed");
                }

                std::vector<signalr::value> args;
                auto argsValues = (*msgpack_obj_index).as<JsonArray>();
                for (JsonVariant ar : argsValues) {
                    args.emplace_back(createValue(ar));
                }

                vec.emplace_back(std::unique_ptr<hub_message>(
                    new invocation_message(std::move(invocation_id), std::move(target), std::move(args))));

                if (arr.size() > 5)
                {
                    // This is for the StreamIds when they are supported
                    ++msgpack_obj_index;
                }

                break;
            }
            case message_type::completion:
            {
                if (arr.size() < 4)
                {
                    throw signalr_exception("completion message has too few properties");
                }

                // HEADERS
                ++msgpack_obj_index;

                if (!(*msgpack_obj_index).is<std::string>())
                {
                    throw signalr_exception("reading 'invocationId' as string failed");
                }
                std::string invocation_id((*msgpack_obj_index).as<std::string>());
                ++msgpack_obj_index;

                if (!(*msgpack_obj_index).is<int>())
                {
                    throw signalr_exception("reading 'result_kind' as int failed");
                }
                auto result_kind = (*msgpack_obj_index).as<int>();
                ++msgpack_obj_index;

                if (arr.size() < 5 && result_kind != 2)
                {
                    throw signalr_exception("completion message has too few properties");
                }

                std::string error;
                signalr::value result;
                // 1: error
                // 2: void result
                // 3: non void result
                if (result_kind == 1)
                {
                    if (!(*msgpack_obj_index).is<std::string>())
                    {
                        throw signalr_exception("reading 'error' as string failed");
                    }
                    error.append((*msgpack_obj_index).as<std::string>());
                }
                else if (result_kind == 3)
                {
                    result = createValue(*msgpack_obj_index);
                }

                vec.emplace_back(std::unique_ptr<hub_message>(
                    new completion_message(std::move(invocation_id), std::move(error), std::move(result), result_kind == 3)));
                break;
            }
            case message_type::ping:
            {
                vec.emplace_back(std::unique_ptr<hub_message>(new ping_message()));
                break;
            }
            // TODO: other message types
            default:
                // Future protocol changes can add message types, old clients can ignore them
                break;
            }

            remaining_message += length_of_message;
            assert(remaining_message_length - length_of_message < remaining_message_length);
            remaining_message_length -= length_of_message;
        }

        return vec;
    }
}
