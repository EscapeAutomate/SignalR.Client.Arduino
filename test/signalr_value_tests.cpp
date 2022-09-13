#include <gtest/gtest.h>
#include "signalrclient/signalr_value.h"

using namespace signalr;

TEST(signalr_value, invalid_cast_throw)
{
    try
    {
        value val(value_type::null);
        val.as_double();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'null' expected it to be a 'float64'", exception.what());
    }

    try
    {
        value val(value_type::array);
        val.as_bool();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'array' expected it to be a 'boolean'", exception.what());
    }

    try
    {
        value val(value_type::map);
        val.as_string();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'map' expected it to be a 'string'", exception.what());
    }

    try
    {
        value val(value_type::string);
        val.as_array();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'string' expected it to be a 'array'", exception.what());
    }

    try
    {
        value val(value_type::float64);
        val.as_map();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'float64' expected it to be a 'map'", exception.what());
    }

    try
    {
        value val(value_type::boolean);
        val.as_binary();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'boolean' expected it to be a 'binary'", exception.what());
    }
}