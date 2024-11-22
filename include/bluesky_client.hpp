#pragma once

#include <vector>
#include <map>

#include <string>

#include <memory>

#include <httplib.h>

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

    enum Error {
        Error_None = 0, // No error
        Error_NotLoggedIn, // Not logged in
        Error_ResponseParseFail, // Failed to parse response JSON
        Error_ResponseFail, // Response was empty or code was not 200
        Error_BadInput, // Bad user input
    };

    bool login(const std::string& identifier, const std::string& password);

    struct PostAuthor {
        std::string did;
        time_t createdAt;

        std::string handle;
        std::string displayName;

        std::string avatarUrl;

        bool blockedByViewer, mutedByViewer;
    };
    
    Error createPost(const std::string& text);

    struct Post {
        std::string uri;
        time_t indexedAt;
        time_t createdAt;

        PostAuthor author;

        std::string cid;

        std::string text;
        
        unsigned int likeCount, quoteCount, replyCount, repostCount;
    };

    struct PostsResult {
        std::vector<Post> posts;
        Error error;
    };

    PostsResult getFeedPosts(const std::string& feedUri, int limit = 1);

    // atId can be either DID or handle
    PostsResult getAuthorPosts(const std::string& atId, int limit = 1);

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
