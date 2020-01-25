//
// Created by munenaga on 2020/01/25.
//

#ifndef HTTP_SERVER_HTTP_CONSTANTS_T_H
#define HTTP_SERVER_HTTP_CONSTANTS_T_H


/**
 * HTTP関連の定数
 */
class http_constants_t {
public:
    /**
     * HTTP の改行コード
     */
    static const std::string CRLF;

    /**
     * HTTP のメッセージボディのデリミタ (CRLF2つ)
     */
    static const std::string CRLF2;

    /**
     * HTTP のヘッダのキー、バリューのデリミタ
     */
    static const std::string HEADER_DELIMITER;
};


#endif //HTTP_SERVER_HTTP_CONSTANTS_T_H
