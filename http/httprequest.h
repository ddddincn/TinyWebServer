#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <errno.h>
#include <mysql/mysql.h>

#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../sqlconnpool/sqlconnRAII.h"

class HttpRequest {
   public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUSET,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    HttpRequest() { init(); }
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer &buff);

    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const char *key) const;
    std::string getPost(const std::string &key) const;

    bool isKeepAlive() const;

   private:
    bool parseRequestLine_(const std::string &line);
    void parseHeader_(const std::string &line);
    void parseBody_(const std::string &line);

    void parsePath_();
    void parsePost_();
    void parseFromUrlencoded_();

    static bool userVerify(const std::string &name, const std::string &pwd, bool isLogin);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int converHex(char ch);
};

#endif  // HTTPREQUEST_H