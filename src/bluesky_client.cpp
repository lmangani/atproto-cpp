#include "bluesky_client.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>

BlueskyClient::BlueskyClient(const std::string& server) : serverHost(server) {
    client = new httplib::SSLClient(serverHost);
    client->set_connection_timeout(10);
    client->set_read_timeout(10);
    client->enable_server_certificate_verification(false);
}

bool BlueskyClient::login(const std::string& username, const std::string& password) {
    // Prepare login payload
    std::stringstream ss;
    ss << "{\"identifier\":\"" << username << "\",\"password\":\"" << password << "\"}";
    
    auto response = makeRequest("POST", "xrpc/com.atproto.server.createSession", {}, ss.str());
    
    if (!response.empty()) {
        // Parse response using yyjson
        yyjson_doc* doc = yyjson_read(response.c_str(), response.length(), 0);
        if (!doc) return false;
        
        yyjson_val* root = yyjson_doc_get_root(doc);
        if (!root) {
            yyjson_doc_free(doc);
            return false;
        }

        // Extract values
        yyjson_val* jwt = yyjson_obj_get(root, "accessJwt");
        yyjson_val* did = yyjson_obj_get(root, "did");
        yyjson_val* handle = yyjson_obj_get(root, "handle");

        if (jwt && did && handle) {
            accessToken = "Bearer " + std::string(yyjson_get_str(jwt));
            userDid = yyjson_get_str(did);
            userHandle = yyjson_get_str(handle);
            yyjson_doc_free(doc);
            return true;
        }
        yyjson_doc_free(doc);
    }
    return false;
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

    auto response = makeRequest("GET", "xrpc/app.bsky.notification.getUnreadCount");
    
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

std::string BlueskyClient::makeRequest(const std::string& method,
                                     const std::string& endpoint,
                                     const std::map<std::string, std::string>& params,
                                     const std::string& body) {
    httplib::Headers headers = {
        {"User-Agent", USER_AGENT},
        {"Content-Type", "application/json"}
    };
    
    if (!accessToken.empty()) {
        headers.emplace("Authorization", accessToken);
    }

    std::string url = endpoint;
    if (!params.empty()) {
        url += "?";
        bool first = true;
        for (const auto& param : params) {
            if (!first) url += "&";
            url += urlEncode(param.first) + "=" + urlEncode(param.second);
            first = false;
        }
    }

    httplib::Result response;
    if (method == "POST") {
        response = client->Post(url.c_str(), headers, body, "application/json");
    } else {
        response = client->Get(url.c_str(), headers);
    }

    if (response && response->status == 200) {
        return response->body;
    }
    
    return "";
}

std::string BlueskyClient::filterText(const std::string& str) {
    std::string out = str;
    
    // Define the replacements using hex values for Unicode characters
    struct CharReplacement {
        const char* from;
        char to;
    };
    
    // List of common Unicode characters to replace
    static const CharReplacement replacements[] = {
        {"\u2018", '\''}, // Left single quote
        {"\u2019", '\''}, // Right single quote
        {"\u201C", '"'},  // Left double quote
        {"\u201D", '"'},  // Right double quote
    };
    
    // Replace Unicode characters with ASCII equivalents
    for (const auto& rep : replacements) {
        size_t pos = 0;
        while ((pos = out.find(rep.from, pos)) != std::string::npos) {
            out.replace(pos, strlen(rep.from), 1, rep.to);
            pos += 1;
        }
    }
    
    // Remove non-ASCII characters
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
