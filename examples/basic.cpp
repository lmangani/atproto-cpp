// basic.cpp
// Bluesky Client example: basic

#include <iostream>

#include "bluesky_client.hpp"

const std::string BSKY_IDENTIFIER = "example.bsky.social";
const std::string BSKY_PASSWORD = "password_goes_here";

int main() {
    BlueskyClient client;

    std::cout << "Bluesky Client example: basic\n\n";

    std::cout << "1. Logging in..";

    if (client.login(BSKY_IDENTIFIER, BSKY_PASSWORD))
        std::cout << " OK\n";
    else {
        std::cout << " FAIL\n";
        return 1;
    }

    std::cout << "2. Get unread notifications..";

    int unread = client.getUnreadCount();

    if (unread >= 0)
        std::cout << " OK (you have " << unread << " unread notification" << (unread == 1 ? "" : "s") << ")\n";
    else {
        std::cout << " FAIL\n";
        return 1;
    }

    std::cout << "3. Get a few posts from the discovery feed..";
    
    auto res = client.getFeedPosts("at://did:plc:z72i7hdynmk6r22z27h6tvur/app.bsky.feed.generator/whats-hot", 3);
    if (res.error == BlueskyClient::Error_None) {
        std::cout << " OK:\n";
        
        for (unsigned i = 0; i < res.posts.size(); i++) {
            const auto& post = res.posts[i];

            char time[64];
            strftime(time, 64, "%a, %d.%m.%Y %H:%M:%S", localtime(&post.createdAt));

            std::cout << "\n    Post no. " << i+1 << ":\n";
            if (!post.author.displayName.empty())
                std::cout << "       - Author: " << post.author.displayName << " (@" << post.author.handle << ")" << '\n';
            else
                std::cout << "       - Author: " << "@" << post.author.handle << '\n';
            std::cout << "       - Date: " << time << '\n';
            std::cout << "       - Text: " <<
                (post.text.empty() ? "(no text)" : BlueskyClient::filterText(post.text)) <<
                '\n';
        }
    }
    else {
        std::cout << " FAIL (" << res.error << ")\n";
        return 1;
    }

    std::cout << "\nThat's all!\n\n";
    
    return 0;
}
