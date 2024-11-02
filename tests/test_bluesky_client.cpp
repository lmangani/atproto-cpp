// tests/test_bluesky_client.cpp
#include <gtest/gtest.h>
#include "bluesky_client.hpp"

class BlueskyClientTest : public ::testing::Test {
protected:
    BlueskyClient client;

    void SetUp() override {
        // Setup code before each test
    }

    void TearDown() override {
        // Cleanup code after each test
    }
};

TEST_F(BlueskyClientTest, FilterTextTest) {
    std::string input = "Hello 'world' with "quotes" and emoji 👋";
    std::string expected = "Hello 'world' with \"quotes\" and emoji ";
    EXPECT_EQ(BlueskyClient::filterText(input), expected);
}

TEST_F(BlueskyClientTest, SplitIntoWordsTest) {
    std::string input = "Hello world with multiple words";
    std::vector<std::string> expected = {"Hello", "world", "with", "multiple", "words"};
    EXPECT_EQ(BlueskyClient::splitIntoWords(input), expected);
}

TEST_F(BlueskyClientTest, LoginFailureTest) {
    EXPECT_FALSE(client.login("invalid-user", "invalid-password"));
    EXPECT_FALSE(client.isLoggedIn());
}

// Mock test for getUnreadCount (requires login)
TEST_F(BlueskyClientTest, UnreadCountNoLoginTest) {
    EXPECT_EQ(client.getUnreadCount(), -1);
}
