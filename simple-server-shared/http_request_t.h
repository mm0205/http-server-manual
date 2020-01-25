//
// Created by munenaga on 2020/01/25.
//

#ifndef HTTP_SERVER_HTTP_REQUEST_T_H
#define HTTP_SERVER_HTTP_REQUEST_T_H

/**
 * HTTP リクエストを表すクラス
 */
class http_request_t {
public:
    /**
     * リクエスト行を取得する
     * @return リクエスト行
     */
    [[nodiscard]] inline const std::string& get_request_line() const {
        return this->request_line;
    }

    /**
     * ヘッダを取得する
     * @return ヘッダ
     */
    [[nodiscard]] inline const std::map<std::string, std::string>& get_header() const {
        return this->header;
    }

    /**
     * ボディを取得する
     * @return ボディ
     */
    [[nodiscard]] inline const std::string get_body() const {
        return this->body;
    }

    /**
     * リクエストのバイト列を追加する
     */
    void add_bytes(const std::string &request_parts);

    /**
     * リクエストの受信が完了して、使える状態か判定する
     *
     * @return リクエストの受信が完了している場合 `true`, それ以外の場合 `false`.
     */
    [[nodiscard]] inline bool is_ready() const {
        return is_header_ready() && is_body_ready();
    }

private:
    /**
     * リクエスト行
     */
    std::string request_line;
    /**
     * メソッド
     */
    std::string method;

    /**
     * URI
     */
    std::string uri;

    /**
     * HTTPバージョン
     */
    std::string http_version;

    /**
     * ヘッダ
     */
    std::map<std::string, std::string> header;

    /**
     * ボディ
     */
    std::string body;

    /**
     * リクエスト全体のバイト列
     */
    std::string total_bytes;

    /**
     * * Content-Length
     * * ヘッダの一部だが、利便性のためにフィールドで持っておく
     */
    int content_length;

    /**
     * 現在のリクエストをパースする
     */
    void try_parse_request();

    /**
     * 全てのヘッダを受信完了したか判定する
     *
     * 受信済みのバイト列の中に CRLF が2つ連続する場合、ヘッダを受信済みと判定する.
     *
     * @return ヘッダを受信済みの場合 `true`, それ以外の場合 `false`.
     */
    [[nodiscard]] bool all_header_received() const;

    /**
     * リクエスト全体をヘッダとボディに分割する
     * @return {ヘッダ, ボディ} のタプル
     */
    std::tuple<std::string, std::string> split_request_to_header_and_body();

    /**
     * 1テキストを改行コード(CRLF)で分割する
     *
     * @return 行文字列のリスト
     */
    std::vector<std::string> split_to_lines(std::string text);

    /**
     * リクエスト行をパースする
     *
     * パース結果はインスタンスの各フィールドにセットする
     *
     * @param request_line リクエスト行文字列
     */
    void parse_request_line(const std::string &request_line);

    /**
     * ヘッダ行をパースする
     *
     * パース結果はインスタンスの `header` マップに登録する
     * キー重複の場合は後の方を無視する。
     *
     * @param header_line ヘッダ行文字列
     */
    void parse_header_line(const std::string &header_line);

    /**
     * ヘッダの受信完了か？
     * @return ヘッダが受信完了の場合 `true`, それ以外の場合 `false`
     */
    [[nodiscard]] bool is_header_ready() const;

    /**
     * ボディの受信完了か?
     * @return ボディが受信完了の場合 `true`, それ以外の場合 `false`.
     */
    [[nodiscard]] bool is_body_ready() const;
};


#endif //HTTP_SERVER_HTTP_REQUEST_T_H
