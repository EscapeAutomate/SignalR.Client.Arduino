#include <gtest/gtest.h>
#include "signalrclient/signalr_value.h"
#include "test_utils.h"

using namespace signalr;



TEST(signalr_value, invalid_cast_throw)
{
    try
    {
        const std::vector<value> vec;
        value val(vec);
        val.as_double();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'array' expected it to be a 'float64'", exception.what());
    }

    try
    {
        const std::vector<value> vec;
        value val(vec);
        val.as_bool();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'array' expected it to be a 'boolean'", exception.what());
    }

    try
    {
        const std::map<std::string, value> map;
        value val(map);
        val.as_string();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'map' expected it to be a 'string'", exception.what());
    }

    try
    {
        const std::string str;        
        value val(str);
        val.as_array();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'string' expected it to be a 'array'", exception.what());
    }

    try
    {
        const double dbl = 10.1;
        value val(dbl);
        val.as_map();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& exception)
    {
        ASSERT_STREQ("object is a 'float64' expected it to be a 'map'", exception.what());
    }
}

TEST(signalr_value, value_copy)
{
    std::vector<value> array;
    const value arrayValue(array);
    value arrayCpy(nullptr);
    arrayCpy = arrayValue;
    assert_signalr_value_equality(arrayValue, arrayCpy);
    
    std::string str = "test";
    const value strValue(str);
    value strCpy(nullptr);
    strCpy = strValue;
    assert_signalr_value_equality(strValue, strCpy);
    
    double dbl = 10.1;
    const value dblValue(dbl);
    value dblCpy(nullptr);
    dblCpy = dblValue;
    assert_signalr_value_equality(dblValue, dblCpy);
    
    bool b = false;
    const value bValue(b);
    value bCpy(nullptr);
    bCpy = bValue;
    assert_signalr_value_equality(bValue, bCpy);
    
    std::map<std::string, value> map = { { "test", value(10.0)} };
    const value mapValue(map);
    value mapCpy(nullptr);
    mapCpy = mapValue;
    assert_signalr_value_equality(mapValue, mapCpy);
}