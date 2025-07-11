//===-- ConnectionFileDescriptorTest.cpp ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TestingSupport/Host/SocketTestUtilities.h"
#include "gtest/gtest.h"
#include "TestingSupport/SubsystemRAII.h"
#include "lldb/Host/posix/ConnectionFileDescriptorPosix.h"
#include "lldb/Utility/UriParser.h"

using namespace lldb_private;

class ConnectionFileDescriptorTest : public testing::Test {
public:
  SubsystemRAII<Socket> subsystems;

  void TestGetURI(std::string ip) {
    std::unique_ptr<TCPSocket> socket_a_up;
    std::unique_ptr<TCPSocket> socket_b_up;
    CreateTCPConnectedSockets(ip, &socket_a_up, &socket_b_up);
    uint16_t socket_a_remote_port = socket_a_up->GetRemotePortNumber();
    ConnectionFileDescriptor connection_file_descriptor(std::move(socket_a_up));

    std::string uri(connection_file_descriptor.GetURI());
    EXPECT_EQ((URI{"connect", ip, socket_a_remote_port, "/"}),
              *URI::Parse(uri));
  }
};

TEST_F(ConnectionFileDescriptorTest, TCPGetURIv4) {
  if (!HostSupportsIPv4())
    return;
  TestGetURI("127.0.0.1");
}

TEST_F(ConnectionFileDescriptorTest, TCPGetURIv6) {
  if (!HostSupportsIPv6())
    return;
  TestGetURI("::1");
}
