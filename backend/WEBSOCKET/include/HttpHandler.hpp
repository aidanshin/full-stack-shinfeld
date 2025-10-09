#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include <string>
#include <map>
#include <sstream> 
#include <iostream>
#include <openssl/sha.h>
#include <openssl/evp.h>


class HttpHandler {
    private:
        struct HttpRequest {
            std::string method;
            std::string path;
            std::string version;
            std::map<std::string, std::string> headers;
            std::string websocketKey;
            std::string origin;
        };

        struct HttpResponse {
            int statusCode;
            std::string statusMessage;
            std::map<std::string, std::string> headers;
            std::string websocketAccept;
        };

        HttpRequest request;
        HttpResponse response;

        bool computeAcceptKey(const std::string& key);        
        bool parseRequest(const std::string& rawRequest);
        bool validateHeader(const std::string& key, const std::string& expectedValue, const char* errMsg);
   public:
        bool validateRequest(const std::string& rawRequest);
        std::string generateResponse();

};

#endif 