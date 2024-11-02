// main.cpp
#include "bluesky_client.hpp"
#include <iostream>

int main() {
    try {
        // Create client instance
        BlueskyClient client;
        
        // Login
        if (!client.login("your-username.bsky.social", "your-password")) {
            std::cerr << "Failed to login" << std::endl;
            return 1;
        }
        
        std::cout << "Logged in as: " << client.getHandle() << std::endl;
        
        // Check unread notifications
        int unread = client.getUnreadCount();
        if (unread >= 0) {
            std::cout << "You have " << unread << " unread notifications" << std::endl;
        }
        
        // Get popular posts
        auto posts = client.getPopularPosts(1);
        if (posts["error"].empty()) {
            std::cout << "\nPopular post:" << std::endl;
            std::cout << "Author: " << posts["name"] << " (@" << posts["handle"] << ")" << std::endl;
            std::cout << "Date: " << posts["date"] << std::endl;
            std::cout << "Text: " << BlueskyClient::filterText(posts["text"]) << std::endl;
        } else {
            std::cerr << "Error getting posts: " << posts["error"] << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
