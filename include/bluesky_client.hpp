#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <httplib.h>
#include <yyjson.h>

class BlueskyClient {
public:
    explicit BlueskyClient(const std::string& server = "bsky.social");
    ~BlueskyClient();

    // Disable copying
    BlueskyClient(const BlueskyClient&) = delete;
    BlueskyClient& operator=(const BlueskyClient&) = delete;

    // Enable moving
    BlueskyClient(BlueskyClient&&);
    BlueskyClient& operator=(BlueskyClient&&);

    // Core functionality
    bool login(const std::string& identifier, const std::string& password);
    bool createPost(const std::string& message);
    std::map<std::string, std::string> getPopularPosts(int limit = 1);
    int getUnreadCount();

    // Helper functions
    static std::string filterText(const std::string& str);
    static std::vector<std::string> splitIntoWords(const std::string& str);
    static std::string urlEncode(const std::string& str);
    static std::string createJsonString(const std::map<std::string, std::string>& data);

    // Status checks and getters
    bool isLoggedIn() const { return !access_token_.empty(); }
    const std::string& getHandle() const { return user_handle_; }
    const std::string& getDid() const { return user_did_; }

private:
    enum RequestMethod {
        RequestMethod_GET = 0,
        RequestMethod_POST,
        RequestMethod_DELETE,

        RequestMethod_Max
    };

    // HTTP request helpers
    std::string makeRequest(
        RequestMethod method,
        const std::string& endpoint,
        const std::map<std::string, std::string>& params = std::map<std::string, std::string>(),
        const std::string& body = std::string()
    );

    // Member variables
    std::unique_ptr<httplib::SSLClient> client_;
    std::string server_host_;
    std::string access_token_;
    std::string user_did_;
    std::string user_handle_;
    std::string refresh_token_;
    
    static const char* const USER_AGENT;
};
