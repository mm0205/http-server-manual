//
// Created by munenaga on 2020/01/25.
//

#ifndef HTTP_SERVER_HTTP_RESPONSE_T_H
#define HTTP_SERVER_HTTP_RESPONSE_T_H

/**
 * HTTP レスポンス
 */
class http_response_t {
public:
    inline void set_status(int _status_code) {
        this->status_code = _status_code;
    }

    inline void add_header(std::string &&key, std::string &&value) {
        this->header.insert(std::make_pair(key, value));
    }

    inline void add_header(const std::string &key, const std::string &value) {
        this->header.insert(std::make_pair(key, value));
    }

    inline void set_body(std::string &&_body) {
        this->body = std::move(_body);
    }

    inline void set_body(const std::string &_body) {
        this->body = _body;
    }

    /**
     * レスポンステキストに変換する
     * @return レスポンステキスト
     */
    std::string to_string() const;

    http_response_t()
        : status_code(200) {
    }

private:
    /**
     * ステータスコード
     */
    int status_code;


    /**
     * HTTP ヘッダ
     */
    std::map<std::string, std::string> header;

    /**
     * レスポンスボディ
     */
    std::string body;
};


#endif //HTTP_SERVER_HTTP_RESPONSE_T_H
