#pragma once

#include <string>
#include <map>
#include <vector>
#include <httplib.h>
#include <yyjson.h>

class BlueskyClient {
public:
    BlueskyClient(const std::string& server = "bsky.social");
    ~BlueskyClient() = default;

    // Core functionality
    bool login(const std::string& username, const std::string& password);
    std::map<std::string, std::string> getPopularPosts(int limit = 1);
    int getUnreadCount();

    // Helper functions
    static std::string filterText(const std::string& str);
    static std::vector<std::string> splitIntoWords(const std::string& str);
    
    // Status checks
    bool isLoggedIn() const { return !accessToken.empty(); }
    std::string getHandle() const { return userHandle; }

private:
    // HTTP request helpers
    std::string makeRequest(const std::string& method,
                           const std::string& endpoint,
                           const std::map<std::string, std::string>& params = {},
                           const std::string& body = "");
    
    std::string buildUrl(const std::string& endpoint);
    std::string urlEncode(const std::string& str);
    
    // Member variables
    httplib::SSLClient* client;
    std::string serverHost;
    std::string accessToken;
    std::string userDid;
    std::string userHandle;
    
    const std::string USER_AGENT = "BlueskyClient/1.0";
};
