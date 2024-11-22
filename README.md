# atproto-cpp

[![CMake Builder](https://github.com/lmangani/atproto-cpp/actions/workflows/cmake-test.yml/badge.svg)](https://github.com/lmangani/atproto-cpp/actions/workflows/cmake-test.yml)

The BlueSky `atproto-cpp` library provides a C++ interface for interacting with the BlueSky social networking service. This library includes functions to handle user authentication, post creation, feed retrieval, and more.

## Features

- User authentication and session management
- Post creation and feed retrieval
- Helper functions for JSON manipulation and URL encoding

## Installation

To use the BlueSky HPP library, include the header files in your project and link against the required dependencies.

```cpp
#include "bluesky_client.hpp"
```

### Usage
#### Initialization
To initialize the BlueskyClient, provide the server address:
```cpp
BlueskyClient client("bsky.social");
```

#### Authentication
To log in to the BlueSky service, use the login function:
```cpp
bool success = client.login("your_identifier", "your_password");
if (success) {
    std::cout << "Logged in successfully!" << std::endl;
} else {
    std::cerr << "Login failed!" << std::endl;
}
```

#### Creating a Post
To create a new post, use the createPost function:
```cpp
BlueskyClient::Error error = client.createPost("Hello, BlueSky!");
if (error == BlueskyClient::Error_None) {
    std::cout << "Post created successfully!" << std::endl;
} else {
    std::cerr << "Failed to create post!" << std::endl;
}
```

#### Retrieving Feed Posts
To retrieve posts from a feed, use the getFeedPosts function:
```cpp
BlueskyClient::PostsResult result = client.getFeedPosts("feed_uri", 10);
if (result.error == BlueskyClient::Error_None) {
    for (const auto& post : result.posts) {
        std::cout << "Post: " << post.text << std::endl;
    }
} else {
    std::cerr << "Failed to retrieve feed posts!" << std::endl;
}
```

#### Retrieving Author Posts
To retrieve posts from a specific author, use the getAuthorPosts function:
```cpp
BlueskyClient::PostsResult result = client.getAuthorPosts("author_id", 10);
if (result.error == BlueskyClient::Error_None) {
    for (const auto& post : result.posts) {
        std::cout << "Author Post: " << post.text << std::endl;
    }
} else {
    std::cerr << "Failed to retrieve author posts!" << std::endl;
}
````

#### Additional Functions

* `getUnreadCount()`: Retrieves the count of unread notifications.
* `filterText(const std::string& str)`: Filters special characters from a text string.
* `splitIntoWords(const std::string& str)`: Splits a string into individual words.
* `urlEncode(const std::string& str)`: Encodes a string for use in URLs.
* `createJsonString(const std::map<std::string, std::string>& data)`: Creates a JSON string from a map of key-value pairs.

### Status
- Prototype, Untested

### Credits
Contains portions of [bluesky_esphom](https://github.com/softplus/bluesky_esphom) and [postbluesky](https://github.com/kenjinote/PostBluesky)
