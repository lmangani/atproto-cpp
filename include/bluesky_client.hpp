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
    bool isLoggedIn() const { return !m_access_token.empty(); }
    const std::string& getHandle() const { return m_user_handle; }
    const std::string& getDid() const { return m_user_did; }

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
    std::unique_ptr<httplib::Client> m_client;
    std::string m_server_host;
    std::string m_access_token;
    std::string m_user_did;
    std::string m_user_handle;
    std::string m_refresh_token;
    
    static const char* const USER_AGENT;
};
