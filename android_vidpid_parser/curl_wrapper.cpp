#include "curl_wrapper.hpp"

// curl init
CurlWrapper::CurlWrapper() : curl{nullptr} {
    try {
        readBuffer.reserve(CURL_MAX_WRITE_SIZE *
                           2);  // string buffer for curl response
    } catch (std::bad_alloc& ba) {
        std::cerr << "Error allocating memory for HTTP response."
                     "bad_alloc caught: "
                  << ba.what() << '\n';
        throw;
    }
    curl = curl_easy_init();
    if (curl == nullptr) {
        std::cerr << "Fatal error found with CURL initialisation" << std::endl;
        throw "Error with cUrl initialization";
    }
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
}

// cleanup CURL
CurlWrapper::~CurlWrapper() { curl_easy_cleanup(curl); }

// callback for CURL
size_t CurlWrapper::writeCallback(void* contents, size_t size, size_t nmemb,
                                  void* userp) {
    //((std::string*)userp)->append((char*)contents, sz * nmemb);
    try {
        static_cast<std::string*>(userp)->append(static_cast<char*>(contents),
                                                 size * nmemb);
    } catch (std::bad_alloc& ba) {
        std::cerr << "Error allocating memory for HTTP response."
                     "bad_alloc caught: "
                  << ba.what() << '\n';
        return 0;
    }
    return size * nmemb;
}

// get Response string for branch
const std::string& CurlWrapper::perfomReq(const std::string& url) {
    readBuffer.clear();
    // std::string req_url=curl_easy_escape(curl,url.c_str(),url.size());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    std::cerr << "Making request to " << url << std::endl;
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        std::cerr << "cUrl request to server failed: \n"
                  << curl_easy_strerror(res) << std::endl;
        readBuffer.clear();
    }
    readBuffer.shrink_to_fit();
    std::cerr << "Downoladed data size: " << readBuffer.size() << std::endl
              << "..." << std::endl;
    return readBuffer;
}
