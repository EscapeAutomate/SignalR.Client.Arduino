// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "signalrclient/json_hub_protocol.h"
#include "test_utils.h"

using namespace signalr;

std::vector<std::pair<std::string, std::shared_ptr<hub_message>>> protocol_test_data
{
    // invocation message without invocation id
    { "{\"type\":1,\"target\":\"Target\",\"arguments\":[1,\"Foo\"]}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(1.f), value("Foo") })) },

    // invocation message with multiple arguments
    { "{\"type\":1,\"invocationId\":\"123\",\"target\":\"Target\",\"arguments\":[1,\"Foo\"]}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("123", "Target", std::vector<value>{ value(1.f), value("Foo") })) },

    // invocation message with bool argument
    { "{\"type\":1,\"target\":\"Target\",\"arguments\":[true]}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(true) })) },

    // invocation message with null argument
    { "{\"type\":1,\"target\":\"Target\",\"arguments\":[null]}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(nullptr) })) },

    // invocation message with no arguments
    { "{\"type\":1,\"target\":\"Target\",\"arguments\":[]}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{})) },

    // invocation message with non-ascii string argument
    /*{ "{\"arguments\":[\"\xD7\x9E\xD7\x97\xD7\xA8\xD7\x95\xD7\x96\xD7\xAA\x20\xD7\x9B\xD7\x9C\xD7\xA9\xD7\x94\xD7\x99\"],\"target\":\"Target\",\"type\":1}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value("\xD7\x9E\xD7\x97\xD7\xA8\xD7\x95\xD7\x96\xD7\xAA\x20\xD7\x9B\xD7\x9C\xD7\xA9\xD7\x94\xD7\x99") })) },*/

    // invocation message with object argument
    { "{\"type\":1,\"target\":\"Target\",\"arguments\":[{\"property\":5}]}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::map<std::string, value>{ {"property", value(5.f)} }) })) },

    // invocation message with array argument
    { "{\"type\":1,\"target\":\"Target\",\"arguments\":[[1,5]]}\x1e",
    std::shared_ptr<hub_message>(new invocation_message("", "Target", std::vector<value>{ value(std::vector<value>{value(1.f), value(5.f)}) })) },

    // ping message
    { "{\"type\":6}\x1e",
    std::shared_ptr<hub_message>(new ping_message()) },

    // completion message with error
    { "{\"type\":3,\"invocationId\":\"1\",\"error\":\"error\"}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "error", value(), false)) },

    // completion message with result
    { "{\"type\":3,\"invocationId\":\"1\",\"result\":42}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "", value(42.f), true)) },

    // completion message with no result or error
    { "{\"type\":3,\"invocationId\":\"1\"}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "", value(), false)) },

    // completion message with null result
    { "{\"type\":3,\"invocationId\":\"1\",\"result\":null}\x1e",
    std::shared_ptr<hub_message>(new completion_message("1", "", value(), true)) },
};

TEST(json_hub_protocol, write_message)
{
    for (auto& data : protocol_test_data)
    {
        auto output = json_hub_protocol().write_message(data.second.get());
        ASSERT_STREQ(data.first.data(), output.data());
    }
}

TEST(json_hub_protocol, parse_message)
{
    for (auto& data : protocol_test_data)
    {
        try
        {
            auto output = json_hub_protocol().parse_messages(data.first);
            ASSERT_EQ(1, output.size());
            assert_hub_message_equality(data.second.get(), output[0].get());
        }
        catch(const std::exception& e)
        {
            ASSERT_TRUE(false) << data.first << " " << e.what() << '\n';
        }
    }
}

TEST(json_hub_protocol, parsing_field_order_does_not_matter)
{
    invocation_message message = invocation_message("123", "Target", std::vector<value>{value(true)});
    auto output = json_hub_protocol().parse_messages("{\"type\":1,\"invocationId\":\"123\",\"arguments\":[true],\"target\":\"Target\"}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());

    output = json_hub_protocol().parse_messages("{\"target\":\"Target\",\"invocationId\":\"123\",\"type\":1,\"arguments\":[true]}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());

    output = json_hub_protocol().parse_messages("{\"target\":\"Target\",\"arguments\":[true],\"type\":1,\"invocationId\":\"123\"}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

TEST(json_hub_protocol, can_parse_multiple_messages)
{
    auto output = json_hub_protocol().parse_messages(std::string("{\"arguments\":[],\"target\":\"Target\",\"type\":1}\x1e") +
        "{\"invocationId\":\"1\",\"result\":42,\"type\":3}\x1e");
    ASSERT_EQ(2, output.size());

    invocation_message invocation = invocation_message("", "Target", std::vector<value>{});
    assert_hub_message_equality(&invocation, output[0].get());

    completion_message completion = completion_message("1", "", value(42.f), true);
    assert_hub_message_equality(&completion, output[1].get());
}

TEST(json_hub_protocol, extra_items_ignored_when_parsing)
{
    invocation_message message = invocation_message("", "Target", std::vector<value>{value(true)});
    auto output = json_hub_protocol().parse_messages("{\"type\":1,\"arguments\":[true],\"target\":\"Target\",\"extra\":\"ignored\"}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

TEST(json_hub_protocol, unknown_message_type_returns_null)
{
    ping_message message = ping_message();
    // adding ping message, just make sure other messages are still being parsed
    auto output = json_hub_protocol().parse_messages("{\"type\":142}\x1e{\"type\":6}\x1e");
    ASSERT_EQ(1, output.size());
    assert_hub_message_equality(&message, output[0].get());
}

std::vector<std::pair<std::string, std::string>> invalid_messages
{
    { "\x1e", "EmptyInput" },
    { "foo\x1e", "IncompleteInput" },
    { "[42]\x1e", "Message was not a 'map' type" },
    { "{\x1e", "IncompleteInput" },

    { "{\"arguments\":[],\"target\":\"send\",\"invocationId\":42}\x1e", "Field 'type' not found" },

    { "{\"type\":1}\x1e", "Field 'target' not found for 'invocation' message" },
    { "{\"type\":1,\"target\":\"send\",\"invocationId\":42}\x1e", "Field 'arguments' not found for 'invocation' message" },
    { "{\"type\":1,\"target\":\"send\",\"arguments\":[],\"invocationId\":42}\x1e", "Expected 'invocationId' to be of type 'string'" },
    { "{\"type\":1,\"target\":\"send\",\"arguments\":42,\"invocationId\":\"42\"}\x1e", "Expected 'arguments' to be of type 'array'" },
    { "{\"type\":1,\"target\":true,\"arguments\":[],\"invocationId\":\"42\"}\x1e", "Expected 'target' to be of type 'string'" },

    { "{\"type\":3}\x1e", "Field 'invocationId' not found for 'completion' message" },
    { "{\"type\":3,\"invocationId\":42}\x1e", "Expected 'invocationId' to be of type 'string'" },
    { "{\"type\":3,\"invocationId\":\"42\",\"error\":[]}\x1e", "Expected 'error' to be of type 'string'" },
    { "{\"type\":3,\"invocationId\":\"42\",\"error\":\"foo\",\"result\":true}\x1e", "The 'error' and 'result' properties are mutually exclusive." },
};

TEST(json_hub_protocol, invalid_messages_throw)
{
    for (auto& pair : invalid_messages)
    {
        try
        {
            json_hub_protocol().parse_messages(pair.first);
            ASSERT_TRUE(false);
        }
        catch (const std::exception& exception)
        {
            ASSERT_STREQ(pair.second.data(), exception.what());
        }
    }
}