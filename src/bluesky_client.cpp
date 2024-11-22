#include "bluesky_client.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <unordered_map>
#include <string>

#include <yyjson.h>

const char* const BlueskyClient::USER_AGENT = "BlueskyClient/1.0";

BlueskyClient::BlueskyClient(const std::string& server) 
    : m_server_host(server)
    , m_client(new httplib::Client("https://" + server))
{
    m_client->set_connection_timeout(10);
    m_client->set_read_timeout(10);
    m_client->enable_server_certificate_verification(false);
    m_client->set_follow_location(true);
}

BlueskyClient::~BlueskyClient() = default;

BlueskyClient::BlueskyClient(BlueskyClient&& other)
    : m_client(std::move(other.m_client))
    , m_server_host(std::move(other.m_server_host))
    , m_access_token(std::move(other.m_access_token))
    , m_user_did(std::move(other.m_user_did))
    , m_user_handle(std::move(other.m_user_handle))
    , m_refresh_token(std::move(other.m_refresh_token))
{}

BlueskyClient& BlueskyClient::operator=(BlueskyClient&& other) {
    if (this != &other) {
        m_client = std::move(other.m_client);
        m_server_host = std::move(other.m_server_host);
        m_access_token = std::move(other.m_access_token);
        m_user_did = std::move(other.m_user_did);
        m_user_handle = std::move(other.m_user_handle);
        m_refresh_token = std::move(other.m_refresh_token);
    }
    return *this;
}

static std::string escapeJson(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.size());

    for (char c : str) {
        switch (c) {
            case '\"': escaped.append("\\\""); break;
            case '\\': escaped.append("\\\\"); break;
            case '\b': escaped.append("\\b"); break;
            case '\f': escaped.append("\\f"); break;
            case '\n': escaped.append("\\n"); break;
            case '\r': escaped.append("\\r"); break;
            case '\t': escaped.append("\\t"); break;
            default: escaped.push_back(c); break;
        }
    }
    return escaped;
}

static time_t datetimeToTimeT(const char* datetime) {
    if (datetime == nullptr)
        return (time_t)(-1);

    struct tm tm {};

    char formattedDatetime[32];
    strncpy(formattedDatetime, datetime, sizeof(formattedDatetime) - 1);
    formattedDatetime[sizeof(formattedDatetime) - 1] = '\0';

    if (strptime(formattedDatetime, "%Y-%m-%dT%H:%M:%S", &tm) == nullptr)
        return (time_t)(-1);

    return timegm(&tm);
}

std::string BlueskyClient::createJsonString(const std::map<std::string, std::string>& data) {
    std::ostringstream ss;

    ss << "{";
    for (auto it = data.begin(); it != data.end(); it++) {
        if (it != data.begin())
            ss << ',';

        ss << "\"" << escapeJson(it->first) << "\":\"" << escapeJson(it->second) << "\"";
    }
    ss << "}";

    return ss.str();
}

bool BlueskyClient::login(const std::string& identifier, const std::string& password) {
    std::map<std::string, std::string> loginData = {
        { "identifier", identifier },
        { "password", password }
    };
    
    auto response = makeRequest(
        RequestMethod_POST, "xrpc/com.atproto.server.createSession", 
        std::map<std::string, std::string>(), 
        createJsonString(loginData)
    );
    
    if (!response.empty()) {
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc)
            return false;
        
        yyjson_val* root = yyjson_doc_get_root(doc);
        if (!root) {
            yyjson_doc_free(doc);
            return false;
        }

        yyjson_val* jwt = yyjson_obj_get(root, "accessJwt");
        yyjson_val* did = yyjson_obj_get(root, "did");
        yyjson_val* handle = yyjson_obj_get(root, "handle");
        yyjson_val* refresh = yyjson_obj_get(root, "refreshJwt");

        if (jwt && did && handle) {
            m_access_token = std::string("Bearer ") + yyjson_get_str(jwt);
            m_user_did = yyjson_get_str(did);
            m_user_handle = yyjson_get_str(handle);
            if (refresh)
                m_refresh_token = yyjson_get_str(refresh);

            yyjson_doc_free(doc);
            return true;
        }

        yyjson_doc_free(doc);
    }

    return false;
}

BlueskyClient::Error BlueskyClient::createPost(const std::string& text) {
    if (!isLoggedIn())
        return Error_NotLoggedIn;
    if (text.empty())
        return Error_BadInput;
    
    // Get current time in ISO 8601 format
    time_t now; time(&now);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    
    std::stringstream ss;
    ss << "{\"collection\":\"app.bsky.feed.post\",\"repo\":\"" 
       << m_user_did 
       << "\",\"record\":{\"text\":\"" 
       << text 
       << "\",\"createdAt\":\"" 
       << timestamp 
       << "\"}}";
    
    auto response = makeRequest(
        RequestMethod_POST, "xrpc/com.atproto.repo.createRecord", 
        std::map<std::string, std::string>(), 
        ss.str()
    );

    if (response.empty())
        return Error_ResponseFail;
    
    return Error_None;
}

BlueskyClient::PostsResult BlueskyClient::getFeedPosts(const std::string& feedUri, int limit) {
    PostsResult result { .error = Error_None };
    if (!isLoggedIn()) {
        result.error = Error_NotLoggedIn;
        return result;
    }

    std::map<std::string, std::string> params = {
        { "feed", feedUri },
        { "limit", std::to_string(limit) }
    };
    auto response = makeRequest(RequestMethod_GET, "xrpc/app.bsky.feed.getFeed", params);

    if (!response.empty()) {
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc) {
            result.error = Error_ResponseParseFail;
            return result;
        }

        yyjson_val* root = yyjson_doc_get_root(doc);
        yyjson_val* feed = yyjson_obj_get(root, "feed");

        unsigned postCount = yyjson_arr_size(feed);

        if (feed && postCount > 0) {
            result.posts.resize(postCount);

            for (unsigned i = 0; i < postCount; i++) {
                yyjson_val* arrEntry = yyjson_arr_get(feed, i);

                auto& outPost = result.posts[i];

                yyjson_val* post = yyjson_obj_get(arrEntry, "post");

                outPost.uri = yyjson_get_str(yyjson_obj_get(post, "uri"));
                outPost.cid = yyjson_get_str(yyjson_obj_get(post, "cid"));

                outPost.replyCount = yyjson_get_uint(yyjson_obj_get(post, "replyCount"));
                outPost.repostCount = yyjson_get_uint(yyjson_obj_get(post, "repostCount"));
                outPost.likeCount = yyjson_get_uint(yyjson_obj_get(post, "likeCount"));
                outPost.quoteCount = yyjson_get_uint(yyjson_obj_get(post, "quoteCount"));

                outPost.indexedAt = datetimeToTimeT(yyjson_get_str(yyjson_obj_get(post, "indexedAt")));
            
                yyjson_val* record = yyjson_obj_get(post, "record");

                outPost.createdAt = datetimeToTimeT(yyjson_get_str(yyjson_obj_get(record, "createdAt")));

                outPost.text = yyjson_get_str(yyjson_obj_get(record, "text"));

                yyjson_val* author = yyjson_obj_get(post, "author");

                outPost.author.did = yyjson_get_str(yyjson_obj_get(author, "did"));

                outPost.author.handle = yyjson_get_str(yyjson_obj_get(author, "handle"));

                const char* displayName = yyjson_get_str(yyjson_obj_get(author, "displayName"));
                if (displayName)
                    outPost.author.displayName = displayName;

                const char* avatarUrl = yyjson_get_str(yyjson_obj_get(author, "avatar"));
                if (avatarUrl)
                    outPost.author.avatarUrl = avatarUrl;

                outPost.author.createdAt = datetimeToTimeT(yyjson_get_str(yyjson_obj_get(author, "createdAt")));
            }
        }
        yyjson_doc_free(doc);
    }
    else
        result.error = Error_ResponseFail;

    return result;
}

BlueskyClient::PostsResult BlueskyClient::getAuthorPosts(const std::string& atId, int limit) {
    PostsResult result { .error = Error_None };
    if (!isLoggedIn()) {
        result.error = Error_NotLoggedIn;
        return result;
    }

    std::map<std::string, std::string> params = {
        { "actor", atId },
        { "limit", std::to_string(limit) },
        { "filter", "posts_no_replies" }
    };
    auto response = makeRequest(RequestMethod_GET, "xrpc/app.bsky.feed.getAuthorFeed", params);

    if (!response.empty()) {
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc) {
            result.error = Error_ResponseParseFail;
            return result;
        }

        yyjson_val* root = yyjson_doc_get_root(doc);
        yyjson_val* feed = yyjson_obj_get(root, "feed");

        unsigned postCount = yyjson_arr_size(feed);

        if (feed && postCount > 0) {
            result.posts.resize(postCount);

            for (unsigned i = 0; i < postCount; i++) {
                yyjson_val* arrEntry = yyjson_arr_get(feed, i);

                auto& outPost = result.posts[i];

                yyjson_val* post = yyjson_obj_get(arrEntry, "post");

                outPost.uri = yyjson_get_str(yyjson_obj_get(post, "uri"));
                outPost.cid = yyjson_get_str(yyjson_obj_get(post, "cid"));

                outPost.replyCount = yyjson_get_uint(yyjson_obj_get(post, "replyCount"));
                outPost.repostCount = yyjson_get_uint(yyjson_obj_get(post, "repostCount"));
                outPost.likeCount = yyjson_get_uint(yyjson_obj_get(post, "likeCount"));
                outPost.quoteCount = yyjson_get_uint(yyjson_obj_get(post, "quoteCount"));

                outPost.indexedAt = datetimeToTimeT(yyjson_get_str(yyjson_obj_get(post, "indexedAt")));
            
                yyjson_val* record = yyjson_obj_get(post, "record");

                outPost.createdAt = datetimeToTimeT(yyjson_get_str(yyjson_obj_get(record, "createdAt")));

                outPost.text = yyjson_get_str(yyjson_obj_get(record, "text"));

                yyjson_val* author = yyjson_obj_get(post, "author");

                outPost.author.did = yyjson_get_str(yyjson_obj_get(author, "did"));

                outPost.author.handle = yyjson_get_str(yyjson_obj_get(author, "handle"));

                const char* displayName = yyjson_get_str(yyjson_obj_get(author, "displayName"));
                if (displayName)
                    outPost.author.displayName = displayName;

                const char* avatarUrl = yyjson_get_str(yyjson_obj_get(author, "avatar"));
                if (avatarUrl)
                    outPost.author.avatarUrl = avatarUrl;

                outPost.author.createdAt = datetimeToTimeT(yyjson_get_str(yyjson_obj_get(author, "createdAt")));
            }
        }
        yyjson_doc_free(doc);
    }
    else
        result.error = Error_ResponseFail;

    return result;
}


int BlueskyClient::getUnreadCount() {
    if (!isLoggedIn())
        return -1;

    auto response = makeRequest(RequestMethod_GET, "xrpc/app.bsky.notification.getUnreadCount");
    
    if (!response.empty()) {
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc)
            return -1;

        yyjson_val* root = yyjson_doc_get_root(doc);
        yyjson_val* count = yyjson_obj_get(root, "count");
        
        int result = yyjson_get_int(count);

        yyjson_doc_free(doc);
        return result;
    }

    return -1;
}

std::string BlueskyClient::makeRequest(
    RequestMethod method,
    const std::string& endpoint,
    const std::map<std::string, std::string>& params,
    const std::string& body
) {
    if (method >= RequestMethod_Max)
        return "";

    httplib::Headers headers = {
        { "User-Agent", USER_AGENT },
        { "Content-Type", "application/json" }
    };
    
    if (!m_access_token.empty())
        headers.emplace("Authorization", m_access_token);

    std::ostringstream sstream;

    sstream << '/' << endpoint;

    if (!params.empty()) {
        sstream << '?';

        for (auto it = params.begin(); it != params.end(); it++) {
            if (it != params.begin())
                sstream << '&';

            sstream << urlEncode(it->first) << '=' << urlEncode(it->second);
        }
    }

    httplib::Result response;
    switch (method) {
    case RequestMethod_GET:
        response = m_client->Get(sstream.str(), headers);
        break;
    case RequestMethod_POST:
        response = m_client->Post(sstream.str(), headers, body, "application/json");
        break;
    case RequestMethod_DELETE:
        response = m_client->Delete(sstream.str(), headers, body, "application/json");
        break;
    
    default:
        return "";
    }

    if (response && response->status == 200)
        return response->body;

    return "";
}

std::string BlueskyClient::filterText(const std::string& str) {
    static const std::unordered_map<wchar_t, char> replacements = {
        { L'\u2018', '\'' }, // Left single quote
        { L'\u2019', '\'' }, // Right single quote
        { L'\u201C', '"'  }, // Left double quote
        { L'\u201D', '"'  }  // Right double quote
    };

    std::string out;
    out.reserve(str.size());

    for (char c : str) {
        auto it = replacements.find(c);
        if (it != replacements.end())
            out.push_back(it->second);
        else if (c >= 32 && c <= 126) // Only keep printable ASCII
            out.push_back(c);
    }

    return out;
}

std::vector<std::string> BlueskyClient::splitIntoWords(const std::string& str) {
    std::vector<std::string> words;
    std::istringstream ss(str);

    std::string word;
    while (ss >> word) {
        if (!word.empty())
            words.push_back(word);
    }
    
    return words;
}

std::string BlueskyClient::urlEncode(const std::string& str) {
    std::string escaped;

    for (unsigned char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            escaped += c;
        else if (c == ' ')
            escaped += '+';
        else {
            escaped += '%';
            escaped += "0123456789ABCDEF"[c / 16];
            escaped += "0123456789ABCDEF"[c % 16];
        }
    }

    return escaped;
}
