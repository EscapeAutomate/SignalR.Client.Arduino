#include "libs/signalrclient/messagepack_hub_protocol.h"
#include "libs/signalrclient/message_type.h"
#include "libs/signalrclient/signalr_exception.h"
#include "libs/signalrclient/binary_message_parser.h"
#include "libs/signalrclient/binary_message_formatter.h"

namespace signalr
{

    std::string messagepack_hub_protocol::write_message(const hub_message* hub_message) const
    {
        string_wrapper str;
        msgpack::packer<string_wrapper> packer(str);

#pragma warning (push)
#pragma warning (disable: 4061)
        switch (hub_message->message_type)
        {
        case message_type::invocation:
        {
            auto invocation = static_cast<invocation_message const*>(hub_message);

            packer.pack_array(6);

            packer.pack_int(static_cast<int>(message_type::invocation));
            // Headers
            packer.pack_map(0);

            if (invocation->invocation_id.empty())
            {
                packer.pack_nil();
            }
            else
            {
                packer.pack_str(static_cast<uint32_t>(invocation->invocation_id.length()));
                packer.pack_str_body(invocation->invocation_id.data(), static_cast<uint32_t>(invocation->invocation_id.length()));
            }

            packer.pack_str(static_cast<uint32_t>(invocation->target.length()));
            packer.pack_str_body(invocation->target.data(), static_cast<uint32_t>(invocation->target.length()));

            packer.pack_array(static_cast<uint32_t>(invocation->arguments.size()));
            for (auto& val : invocation->arguments)
            {
                pack_messagepack(val, packer);
            }

            // StreamIds
            packer.pack_array(0);

            break;
        }
        case message_type::completion:
        {
            auto completion = static_cast<completion_message const*>(hub_message);

            size_t result_kind = completion->error.empty() ? (completion->has_result ? 3U : 2U) : 1U;
            packer.pack_array(4U + (result_kind != 2U ? 1U : 0U));

            packer.pack_int(static_cast<int>(message_type::completion));

            // Headers
            packer.pack_map(0);

            packer.pack_str(static_cast<uint32_t>(completion->invocation_id.length()));
            packer.pack_str_body(completion->invocation_id.data(), static_cast<uint32_t>(completion->invocation_id.length()));

            packer.pack_int(static_cast<int>(result_kind));
            switch (result_kind)
            {
                // error result
            case 1:
                packer.pack_str(static_cast<uint32_t>(completion->error.length()));
                packer.pack_str_body(completion->error.data(), static_cast<uint32_t>(completion->error.length()));
                break;
                // non-void result
            case 3:
                pack_messagepack(completion->result, packer);
                break;
            }

            break;
        }
        case message_type::ping:
        {
            // If we need the ping this is how you get it
            // auto ping = static_cast<ping_message const*>(hub_message);

            packer.pack_array(1);
            packer.pack_int(static_cast<int>(message_type::ping));

            break;
        }
        default:
            break;
        }
#pragma warning (pop)

        binary_message_formatter::write_length_prefix(str.str);
        return str.str;
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

            msgpack::unpacker pac;
            pac.reserve_buffer(length_of_message);
            memcpy(pac.buffer(), remaining_message, length_of_message);
            pac.buffer_consumed(length_of_message);
            msgpack::object_handle obj_handle;

            if (!pac.next(obj_handle))
            {
                throw signalr_exception("messagepack object was incomplete");
            }

            auto& msgpack_obj = obj_handle.get();

            if (msgpack_obj.type != msgpack::type::ARRAY)
            {
                throw signalr_exception("Message was not an 'array' type");
            }

            auto num_elements_of_message = msgpack_obj.via.array.size;
            if (msgpack_obj.via.array.size == 0)
            {
                throw signalr_exception("Message was an empty array");
            }

            auto msgpack_obj_index = msgpack_obj.via.array.ptr;
            if (msgpack_obj_index->type != msgpack::type::POSITIVE_INTEGER)
            {
                throw signalr_exception("reading 'type' as int failed");
            }
            auto type = msgpack_obj_index->via.i64;
            ++msgpack_obj_index;

#pragma warning (push)
#pragma warning (disable: 4061)
            switch (static_cast<message_type>(type))
            {
            case message_type::invocation:
            {
                if (num_elements_of_message < 5)
                {
                    throw signalr_exception("invocation message has too few properties");
                }

                // HEADERS
                ++msgpack_obj_index;

                std::string invocation_id;
                if (msgpack_obj_index->type == msgpack::type::STR)
                {
                    invocation_id.append(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
                }
                else if (msgpack_obj_index->type != msgpack::type::NIL)
                {
                    throw signalr_exception("reading 'invocationId' as string failed");
                }
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::STR)
                {
                    throw signalr_exception("reading 'target' as string failed");
                }

                std::string target(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::ARRAY)
                {
                    throw signalr_exception("reading 'arguments' as array failed");
                }

                std::vector<signalr::value> args;
                auto size = msgpack_obj_index->via.array.size;
                auto arg_array_index = msgpack_obj_index->via.array.ptr;
                for (uint32_t i = 0; i < size; ++i)
                {
                    args.emplace_back(createValue(*arg_array_index));
                    ++arg_array_index;
                }

                vec.emplace_back(std::unique_ptr<hub_message>(
                    new invocation_message(std::move(invocation_id), std::move(target), std::move(args))));

                if (num_elements_of_message > 5)
                {
                    // This is for the StreamIds when they are supported
                    ++msgpack_obj_index;
                }

                break;
            }
            case message_type::completion:
            {
                if (num_elements_of_message < 4)
                {
                    throw signalr_exception("completion message has too few properties");
                }

                // HEADERS
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::STR)
                {
                    throw signalr_exception("reading 'invocationId' as string failed");
                }
                std::string invocation_id(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
                ++msgpack_obj_index;

                if (msgpack_obj_index->type != msgpack::type::POSITIVE_INTEGER)
                {
                    throw signalr_exception("reading 'result_kind' as int failed");
                }
                auto result_kind = msgpack_obj_index->via.i64;
                ++msgpack_obj_index;

                if (num_elements_of_message < 5 && result_kind != 2)
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
                    if (msgpack_obj_index->type != msgpack::type::STR)
                    {
                        throw signalr_exception("reading 'error' as string failed");
                    }
                    error.append(msgpack_obj_index->via.str.ptr, msgpack_obj_index->via.str.size);
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
#pragma warning (pop)

            remaining_message += length_of_message;
            assert(remaining_message_length - length_of_message < remaining_message_length);
            remaining_message_length -= length_of_message;
        }

        return vec;
    }
}
