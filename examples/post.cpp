// post.cpp
// Bluesky Client example: creating a post

#include <iostream>

#include "bluesky_client.hpp"

const std::string BSKY_IDENTIFIER = "example.bsky.social";
const std::string BSKY_PASSWORD = "password_goes_here";

int main() {
    std::cout << "Bluesky Client example: creating a post\n\n";

    BlueskyClient client;

    std::cout << "1. Logging in..";

    if (client.login(BSKY_IDENTIFIER, BSKY_PASSWORD))
        std::cout << " OK\n";
    else {
        std::cout << " FAIL\n";
        return 1;
    }

    const std::string postText = "Hello from the C++11 BlueSky client!";

    std::cout << "2. Create post with text: \"" << postText << "\"..";

    if (client.createPost(postText) == BlueskyClient::Error_None)
        std::cout << " OK\n";
    else {
        std::cout << " FAIL\n";
        return 1;
    }

    std::cout << "3. Confirm the post..";

    auto res = client.getAuthorPosts(client.getDid(), 1);
    if (res.error == BlueskyClient::Error_None) {
        const auto& post = res.posts[0];

        const bool failed = post.text != postText;

        if (failed)
            std::cout << " FAIL (last post text is nonmatching):\n";
        else
            std::cout << " OK:\n";

        char time[64];
        strftime(time, 64, "%a, %d.%m.%Y %H:%M:%S", localtime(&post.createdAt));

        std::cout << "\n    Last post:\n";
        if (!post.author.displayName.empty())
            std::cout << "       - Author: " << post.author.displayName << " (@" << post.author.handle << ")" << '\n';
        else
            std::cout << "       - Author: " << "@" << post.author.handle << '\n';
        std::cout << "       - Date: " << time << '\n';
        std::cout << "       - Text: " <<
            (post.text.empty() ? "(no text)" : BlueskyClient::filterText(post.text)) <<
            '\n';

        if (failed)
            return 1;
    }
    else {
        std::cout << " FAIL (" << res.error << ")\n";
        return 1;
    }

    std::cout << "\nThat's all!\n\n";
}