#ifndef CURLWRAPPER_H
#define CURLWRAPPER_H
#include <curl/curl.h>

#include <iostream>
#include <string>

class CurlWrapper {
   public:
    CurlWrapper();  // curl init
    CurlWrapper(const CurlWrapper&) = delete;
    CurlWrapper& operator=(const CurlWrapper&) = delete;
    ~CurlWrapper();

    // perfom request to server, returns json response string
    const std::string& perfomReq(const std::string& url);

    // callback for curl
    static size_t writeCallback(void* contents, size_t sz, size_t nmemb,
                                void* userp);

   private:
    std::string readBuffer;
    CURL* curl;
};

#endif  // CURLWRAPPER_H
