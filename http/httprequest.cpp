#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpRequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::parse(Buffer &buff) {
    const char CRLF[] = "\r\n";
    if (buff.readableBytes() <= 0) {
        return false;
    }
    while (buff.readableBytes() && state_ != FINISH) {
        const char *lineEnd = std::search(buff.peek(), buff.peek() + buff.readableBytes(), CRLF, CRLF + 2);  // 从缓存区中找到行结束位置
        std::string line(buff.peek(), lineEnd);                                                              // 从buff中取出一行

        switch (state_) {
            case REQUEST_LINE:
                if (!parseRequestLine_(line)) {
                    return false;
                }
                parsePath_();
                break;
            case HEADERS:
                parseHeader_(line);
                if (buff.readableBytes() <= 2) {
                    state_ = FINISH;
                }
                break;
            case BODY:
                parseBody_(line);
                break;
            default:
                break;
        }
        if (lineEnd == buff.beginWrite()) {
            break;
        }
        buff.retrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

std::string HttpRequest::path() const {
    return path_;
}

std::string &HttpRequest::path() {
    return path_;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::getPost(const char *key) const {
    assert(key != nullptr);
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::getPost(const std::string &key) const {
    assert(key != "");
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

bool HttpRequest::isKeepAlive() const {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parseRequestLine_(const std::string &line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");  // 匹配类似于 "GET /index.html HTTP/1.1"
    std::smatch subMatch;                                 // 用于存放正则匹配结果
    if (std::regex_match(line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine ERROR");
    return false;
}

void HttpRequest::parseHeader_(const std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");  // 匹配类似于 "key: value"
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = BODY;
    }
}

void HttpRequest::parseBody_(const std::string &line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::parsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        for (auto &item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

void HttpRequest::parsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        parseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("TAG:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (userVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::parseFromUrlencoded_() {
    if (body_.size() == 0) {
        return;
    }

    std::string key, value;
    int i = 0, j = 0;
    int num = 0;

    for (i; i < body_.size(); i++) {
        char ch = body_[i];
        switch (ch) {
            case '=':
                key = body_.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = converHex(body_[i + 1]) * 16 + converHex(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::userVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if (name == "" || pwd == "") {
        return false;
    }
    LOG_INFO("Verify name: %s ,password: %s ", name.c_str(), pwd.c_str());

    MYSQL *conn;
    SqlConnRAII(&conn, SqlConnPool::instance());
    assert(conn);

    bool flag;
    unsigned int j = 0;
    char sql[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin) {
        flag = true;
    }

    snprintf(sql, 256, "SELECT username, password FROM user WHERE username = '%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", sql);

    if (mysql_query(conn, sql) && isLogin) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(conn);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);

        if (isLogin) {
            if (pwd == password) {
                LOG_INFO("%s LOGIN SUCCESS", row[0]);
                return true;
            } else {
                flag = false;
                LOG_INFO("%s PASSWORD ERROR", row[0]);
            }
        } else {
            flag = false;
            LOG_INFO("%s USERNAME USED", row[0]);
        }
    }
    mysql_free_result(res);

    if (!isLogin && flag) {
        LOG_DEBUG("register");
        bzero(sql, 256);
        snprintf(sql, 256, "INSERT INTO user(username,password) VALUEs('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", sql);
        if (mysql_query(conn, sql)) {
            LOG_DEBUG("INSERT ERROE");
            flag = false;
        }
        flag = true;
    }
    LOG_DEBUG("verify success");
    return true;
}

int HttpRequest::converHex(char ch) {
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch - '0';
}