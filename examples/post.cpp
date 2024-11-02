#include "bluesky_client.hpp"
#include <iostream>

int main() {
    try {
        BlueskyClient client;
        
        if (client.login("username", "password")) {
            // Create a post
            client.createPost("Hello from the C++11 BlueSky client!");
            
            // Get popular posts
            auto posts = client.getPopularPosts(5);
            if (posts["error"].empty()) {
                std::cout << "Popular post by: " << posts["handle"] << "\n";
                std::cout << "Text: " << posts["text"] << "\n";
            }
            
            // Check notifications
            int unread = client.getUnreadCount();
            if (unread >= 0) {
                std::cout << "You have " << unread << " unread notifications\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
}
