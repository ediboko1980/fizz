/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>

#include <fizz/crypto/test/TestUtil.h>
#include <fizz/server/TicketTypes.h>
#include <fizz/util/FizzUtil.h>
#include <folly/FileUtil.h>
#include <folly/experimental/TestUtil.h>

using namespace folly;
using namespace testing;

namespace fizz {
namespace test {

TEST(UtilTest, GetAlpnFromNpn) {
  std::list<folly::SSLContext::NextProtocolsItem> npList;
  std::list<std::string> protocolList1{"test", "test2"};
  std::list<std::string> protocolList2{"test3", "test4"};

  npList.push_back(folly::SSLContext::NextProtocolsItem(1, protocolList1));
  {
    std::vector<std::string> expectedList{std::begin(protocolList1),
                                          std::end(protocolList1)};
    EXPECT_EQ(FizzUtil::getAlpnsFromNpnList(npList), expectedList);
  }

  npList.push_back(folly::SSLContext::NextProtocolsItem(2, protocolList2));
  {
    std::vector<std::string> expectedList{std::begin(protocolList2),
                                          std::end(protocolList2)};
    EXPECT_EQ(FizzUtil::getAlpnsFromNpnList(npList), expectedList);
  }
}

TEST(UtilTest, CreateTickerCipher) {
  auto cipher = FizzUtil::createTicketCipher<server::AES128TicketCipher>(
      std::vector<std::string>(),
      "fakeSecretttttttttttttttttttttttttt",
      std::vector<std::string>(),
      // any number high enough to last the duration of the test should be fine
      std::chrono::seconds(100),
      folly::Optional<std::string>("fakeContext"));
  {
    server::ResumptionState state;
    auto blob = cipher->encrypt(std::move(state)).get();
    EXPECT_EQ(
        std::get<0>(cipher->decrypt(std::move(std::get<0>(*blob))).get()),
        PskType::Resumption);
  }
  {
    auto newCipher = FizzUtil::createTicketCipher<server::AES128TicketCipher>(
        std::vector<std::string>(),
        "fakeSecrettttttttttttttttttttttttt2",
        std::vector<std::string>(),
        std::chrono::seconds(100),
        folly::Optional<std::string>("fakeContext"));
    server::ResumptionState state;
    auto blob = cipher->encrypt(std::move(state)).get();
    EXPECT_EQ(
        std::get<0>(newCipher->decrypt(std::move(std::get<0>(*blob))).get()),
        PskType::Rejected);
  }
}

TEST(UtilTest, ReadPKey) {
  {
    folly::test::TemporaryFile testFile("test");
    folly::writeFileAtomic(testFile.path().native(), kP256Key);
    FizzUtil::readPrivateKey(testFile.path().native(), "");
  }
  {
    folly::test::TemporaryFile testFile("test");
    folly::writeFileAtomic(
        testFile.path().native(), folly::StringPiece("test"));
    EXPECT_THROW(
        FizzUtil::readPrivateKey(testFile.path().native(), ""),
        std::runtime_error);
  }
}

TEST(UtilTest, ReadChainFile) {
  {
    folly::test::TemporaryFile testFile("test");
    folly::writeFileAtomic(testFile.path().native(), kP256Certificate);
    EXPECT_EQ(FizzUtil::readChainFile(testFile.path().native()).size(), 1);
  }
  {
    folly::test::TemporaryFile testFile("test");
    folly::writeFileAtomic(testFile.path().native(), kP384Key);
    EXPECT_THROW(
        FizzUtil::readChainFile(testFile.path().native()), std::runtime_error);
  }
  {
    EXPECT_THROW(
        FizzUtil::readChainFile("test_file_does_not_exist"),
        std::runtime_error);
  }
}

} // namespace test
} // namespace fizz
