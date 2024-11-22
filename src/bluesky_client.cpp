#include "bluesky_client.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

const char* const BlueskyClient::USER_AGENT = "BlueskyClient/1.0";

BlueskyClient::BlueskyClient(const std::string& server) 
    : server_host_(server)
    , client_(new httplib::SSLClient(server_host_)) {
    
    client_->set_connection_timeout(10);
    client_->set_read_timeout(10);
    client_->enable_server_certificate_verification(false);
}

BlueskyClient::~BlueskyClient() = default;

BlueskyClient::BlueskyClient(BlueskyClient&& other)
    : client_(std::move(other.client_))
    , server_host_(std::move(other.server_host_))
    , access_token_(std::move(other.access_token_))
    , user_did_(std::move(other.user_did_))
    , user_handle_(std::move(other.user_handle_))
    , refresh_token_(std::move(other.refresh_token_)) {}

BlueskyClient& BlueskyClient::operator=(BlueskyClient&& other) {
    if (this != &other) {
        client_ = std::move(other.client_);
        server_host_ = std::move(other.server_host_);
        access_token_ = std::move(other.access_token_);
        user_did_ = std::move(other.user_did_);
        user_handle_ = std::move(other.user_handle_);
        refresh_token_ = std::move(other.refresh_token_);
    }
    return *this;
}

std::string BlueskyClient::createJsonString(const std::map<std::string, std::string>& data) {
    std::stringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& pair : data) {
        if (!first) {
            ss << ",";
        }
        ss << "\"" << pair.first << "\":\"" << pair.second << "\"";
        first = false;
    }
    ss << "}";
    return ss.str();
}

bool BlueskyClient::login(const std::string& identifier, const std::string& password) {
    std::map<std::string, std::string> loginData = {
        {"identifier", identifier},
        {"password", password}
    };
    
    auto response = makeRequest(
        RequestMethod_POST, "xrpc/com.atproto.server.createSession", 
        std::map<std::string, std::string>(), 
        createJsonString(loginData)
    );
    
    if (!response.empty()) {
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc) return false;
        
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
            access_token_ = "Bearer " + std::string(yyjson_get_str(jwt));
            user_did_ = yyjson_get_str(did);
            user_handle_ = yyjson_get_str(handle);
            if (refresh) {
                refresh_token_ = yyjson_get_str(refresh);
            }
            yyjson_doc_free(doc);
            return true;
        }
        yyjson_doc_free(doc);
    }
    return false;
}

bool BlueskyClient::createPost(const std::string& message) {
    if (!isLoggedIn() || message.empty()) {
        return false;
    }
    
    // Get current time in ISO 8601 format
    time_t now;
    time(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    
    std::stringstream ss;
    ss << "{\"collection\":\"app.bsky.feed.post\",\"repo\":\"" 
       << user_did_ 
       << "\",\"record\":{\"text\":\"" 
       << message 
       << "\",\"createdAt\":\"" 
       << timestamp 
       << "\"}}";
    
    auto response = makeRequest(
        RequestMethod_POST, "xrpc/com.atproto.repo.createRecord", 
        std::map<std::string, std::string>(), 
        ss.str()
    );

    return !response.empty();
}

std::map<std::string, std::string> BlueskyClient::getPopularPosts(int limit) {
    std::map<std::string, std::string> result;
    if (!isLoggedIn()) {
        result["error"] = "Not logged in";
        return result;
    }

    std::map<std::string, std::string> params = {{"limit", std::to_string(limit)}};
    auto response = makeRequest("GET", "xrpc/app.bsky.unspecced.getPopular", params);

    if (!response.empty()) {
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc) {
            result["error"] = "Failed to parse response";
            return result;
        }

        yyjson_val* root = yyjson_doc_get_root(doc);
        yyjson_val* feed = yyjson_obj_get(root, "feed");
        if (feed && yyjson_arr_size(feed) > 0) {
            yyjson_val* first_post = yyjson_arr_get(feed, 0);
            yyjson_val* post = yyjson_obj_get(first_post, "post");
            yyjson_val* author = yyjson_obj_get(post, "author");
            yyjson_val* record = yyjson_obj_get(post, "record");

            result["handle"] = yyjson_get_str(yyjson_obj_get(author, "handle"));
            result["name"] = yyjson_get_str(yyjson_obj_get(author, "displayName"));
            result["date"] = yyjson_get_str(yyjson_obj_get(record, "createdAt"));
            result["text"] = yyjson_get_str(yyjson_obj_get(record, "text"));
            result["error"] = "";
        }
        yyjson_doc_free(doc);
    } else {
        result["error"] = "No data received";
    }
    return result;
}

int BlueskyClient::getUnreadCount() {
    if (!isLoggedIn()) return -1;

    auto response = makeRequest(RequestMethod_GET, "xrpc/app.bsky.notification.getUnreadCount");
    
    if (!response.empty()) {
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc) return -1;

        yyjson_val* root = yyjson_doc_get_root(doc);
        yyjson_val* count = yyjson_obj_get(root, "count");
        
        int result = count ? yyjson_get_int(count) : -1;
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
    }
    
    return "";
}

std::string BlueskyClient::filterText(const std::string& str) {
    std::string out = str;
    
    struct CharReplacement {
        const char* from;
        char to;
    };
    
    static const CharReplacement replacements[] = {
        {"\u2018", '\''}, // Left single quote
        {"\u2019", '\''}, // Right single quote
        {"\u201C", '"'},  // Left double quote
        {"\u201D", '"'},  // Right double quote
    };
    
    for (const auto& rep : replacements) {
        size_t pos = 0;
        while ((pos = out.find(rep.from, pos)) != std::string::npos) {
            out.replace(pos, strlen(rep.from), 1, rep.to);
            pos += 1;
        }
    }
    
    out.erase(std::remove_if(out.begin(), out.end(),
        [](unsigned char ch) { return (ch < 32) || (ch > 126); }), out.end());
    
    return out;
}

std::vector<std::string> BlueskyClient::splitIntoWords(const std::string& str) {
    std::vector<std::string> words;
    std::stringstream ss(str);
    std::string word;
    
    while (ss >> word) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    
    return words;
}

std::string BlueskyClient::urlEncode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << '+';
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}
