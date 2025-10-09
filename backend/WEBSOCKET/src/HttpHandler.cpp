#include "HttpHandler.hpp"
#include "Logger.hpp"

static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), 
        [](int c) { return !std::isspace(c); }));
    return s;
}

static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](int c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}


bool HttpHandler::parseRequest(const std::string& rawRequest) {
    std::istringstream stream(rawRequest);
    std::string line;

    if(!std::getline(stream, line)) RETURN_ERROR("HttpHandler[parseRequest] - no first line found getline()");
    std::istringstream firstLine(line);
    if(!(firstLine >> request.method >> request.path >> request.version)) RETURN_ERROR("HttpHandler[parseRequest] - invalid format for first line");
    
    while(std::getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colon = line.find(':');
        if(colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon+1);
        trim(key);
        trim(value);
        if(request.websocketKey.empty() && key == "Sec-WebSocket-Key") request.websocketKey = value;
        else if (request.origin.empty() && key == "Origin") request.origin = value;
        else request.headers[key] = value;
    }

    return true;
}

bool HttpHandler::validateHeader(const std::string& key, const std::string& expectedValue, const char* errMsg) {
    auto it = request.headers.find(key);
    if(it == request.headers.end() || it->second != expectedValue) {
        RETURN_ERROR("HttpHandler[validateheader] - %s: %s", errMsg, it != request.headers.end() ? it->second.c_str() : "<missing>");
    }
    return true;
}

bool HttpHandler::validateRequest(const std::string& rawRequest) {
    if(!parseRequest(rawRequest)) RETURN_WARNING("HttpHandler[validateRequest] - Error within parseRequest");
    if(request.method != "GET") RETURN_ERROR("HttpHandler[validateRequest] - Invalid Method: %s", request.method.c_str());
    if(request.version != "HTTP/1.1") RETURN_ERROR("HttpHandler[validateRequest] - Invalid Version: %s", request.version.c_str());

    if(!validateHeader("Upgrade", "websocket", "Invalid Upgrade")) return false;
    if(!validateHeader("Connection", "Upgrade", "Invalid Connection")) return false;
    if(!validateHeader("Sec-WebSocket-Version", "13", "Invalid Sec-WebSocket-Version")) return false;

    if(request.websocketKey.empty() || request.websocketKey.length() != 24) RETURN_ERROR("HttpHandler[validateRequest] - Empty websocketkey");

    return true;
}

std::string HttpHandler::generateResponse() {

    response.statusCode = 101;
    response.statusMessage = "Switching Protocols";
    response.headers["Upgrade"] = "websocket";
    response.headers["Connection"] = "Upgrade";
    if (!computeAcceptKey(request.websocketKey)) {
        ERROR_SRC("HttpHandler[generateResponse] - Invalid key calculation");
        return "";
    }

    std::ostringstream oss;
    oss << request.version << " " << response.statusCode << " " << response.statusMessage << "\r\n";
    for(auto &[k, v] : response.headers) {
        oss << k << ": " << v << "\r\n";
    }
    oss << "Sec-WebSocket-Accept: " << response.websocketAccept << "\r\n";
    oss << "\r\n";
    return oss.str();
}

bool HttpHandler::computeAcceptKey(const std::string& key) {
    static constexpr char magic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::string concat = key + magic;

    unsigned char sha1_digest[SHA_DIGEST_LENGTH];
    if(!SHA1(reinterpret_cast<const unsigned char*>(concat.data()), concat.size(), sha1_digest)) {
        ERROR_SRC("HttpHandler[computeAcceptKey] - Error within SHA1");
        return false;
    }
    
    int encoded_len = 4 * ((SHA_DIGEST_LENGTH + 2) / 3);
    std::vector<unsigned char> base64_output(encoded_len + 1);

    int written = EVP_EncodeBlock(base64_output.data(), sha1_digest, SHA_DIGEST_LENGTH);
    if(written != encoded_len) WARNING_SRC("HttpHandler[computeAcceptKey] - encoded sha1 is not expected length of %i | %i", encoded_len, written);

    response.websocketAccept = std::string(reinterpret_cast<char*>(base64_output.data()), written);

    return true;
}

