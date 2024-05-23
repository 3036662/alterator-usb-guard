pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
message("CURL_FOUND: ${CURL_FOUND}")

add_executable(vid_parser 
main.cpp
curl_wrapper.cpp
)
target_link_libraries(vid_parser PRIVATE boost_json)
target_link_libraries(vid_parser PRIVATE PkgConfig::CURL)

