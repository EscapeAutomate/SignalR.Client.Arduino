/*
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
**/
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <SignalR_Client_Arduino.h>
#include <signalrclient/messagepack_hub_protocol.h>

using namespace signalr;

TEST(messagepack_hub_protocol, SerializeInvocation)
{
	auto mhp = new messagepack_hub_protocol();
	std::vector<signalr::value> args { 42.0 };
	invocation_message invocation("", "method", args);
	auto message = mhp->write_message(&invocation);

	uint8_t goodPayload[] = { 0x96, 0x01, 0x80, 0xc0, 0xa6, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64, 0x91, 0x2a, 0x90 };

	for (int i = 1; i < message.length(); ++i) {
		EXPECT_EQ(goodPayload[i-1], (uint8_t)message[i]) << "Vectors x and y differ at index " << i;
	}
}

TEST(messagepack_hub_protocol, SerializeInvocationWithInvocationId)
{
	auto mhp = new messagepack_hub_protocol();
	std::vector<signalr::value> args { 42.0 };
	invocation_message invocation("xyz", "method", args);
	auto message = mhp->write_message(&invocation);

	uint8_t goodPayload[] = { 0x96, 0x01, 0x80, 0xa3, 0x78, 0x79, 0x7a, 0xa6, 0x6d, 0x65, 0x74, 0x68, 0x6f, 0x64, 0x91, 0x2a, 0x90 };

	for (int i = 1; i < message.length(); ++i) {
		EXPECT_EQ(goodPayload[i-1], (uint8_t)message[i]) << "Vectors x and y differ at index " << i;
	}
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
	if (RUN_ALL_TESTS())
	;
	// Always return zero-code and allow PlatformIO to parse results
	return 0;
}