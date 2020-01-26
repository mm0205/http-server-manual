//
// Created by munenaga on 2020/01/25.
//

#include "common.h"
#include <boost/algorithm/string.hpp>
#include "http_request_t.h"
#include "http_constants_t.h"

void http_request_t::add_bytes(const std::string &request_parts) {

    if (request_parts.empty()) {
        return;
    }
    // ヘッダが ready でない場合は ヘッダの読み込み処理を行う
    if (!this->is_header_ready()) {

        // リクエスト全体のバイト列に今回のバイト列を追加する
        this->total_bytes.insert(
            this->total_bytes.end(),
            request_parts.begin(),
            request_parts.end()
        );

        // 試しにヘッダをパースしてみる
        this->try_parse_request();
        return;
    }

    // ボディがが ready でない場合
    if (!this->is_body_ready()) {
        // ボディに文字列を追加する
        this->body.append(request_parts);
    }

    throw std::runtime_error("すでにリクエストを受信済みです。");
}

void http_request_t::try_parse_request() {
    // まだヘッダ全体を受信できていない場合は何もしない
    if (!this->all_header_received()) {
        return;
    }

    // デリミタでボディ部分と分割する
    std::string header_text;
    std::string body_text;
    std::tie(header_text, body_text) = this->split_request_to_header_and_body();

    // ヘッダ部分を各行に分割する
    std::vector<std::string> lines = this->split_to_lines(header_text);
    if (lines.empty()) {
        // Request-line は必須
        throw std::runtime_error("リクエストにRequest-line が含まれていません");
    }

    // 先頭行が Request-line
    auto it = lines.begin();
    this->parse_request_line(*it);

    // 先頭行の次の行 〜 Body の直前までがヘッダなのでパースする
    it++;
    for (; it != lines.end(); it++) {
        this->parse_header_line(*it);
    }

    // ヘッダに Content-Length が含まれる場合は Body の読み込み終了判定に必要なので、
    // 保存しておく。
    auto content_length_it = this->header.find("Content-Length");
    if (content_length_it != this->header.end()) {
        const auto content_length_text = boost::trim_copy(content_length_it->second);
        this->content_length = std::stoi(content_length_text);
    }

    // ヘッダ以降の部分は全て ボディ
    this->body.append(body_text);
}

bool http_request_t::all_header_received() const {
    const auto size_ok = this->total_bytes.size() >= http_constants_t::CRLF2.size();
    const auto found = this->total_bytes.find(http_constants_t::CRLF2) != std::string::npos;
    return size_ok && found;
}


std::tuple<std::string, std::string> http_request_t::split_request_to_header_and_body() {
    auto result = boost::find_first(this->total_bytes, http_constants_t::CRLF2);
    if (!result) {
        throw std::runtime_error("Request delimiter not found.");
    }


    const std::string header_text(this->total_bytes.begin(), result.begin());
    const std::string body_text(result.end(), this->total_bytes.end());

    return std::make_tuple(header_text, body_text);
}

std::vector<std::string> http_request_t::split_to_lines(std::string text) {
    std::vector<std::string> result;
    boost::split(result, text, boost::is_any_of(http_constants_t::CRLF), boost::token_compress_off);
    return result;
}

void http_request_t::parse_request_line(const std::string &request_line) {
    std::vector<std::string> parts;
    boost::split(parts, request_line, boost::is_any_of(" "), boost::token_compress_off);

    if (parts.size() != 3) {
        throw std::runtime_error("Request-line のフィールド数が3ではありません。");
    }

    this->request_line = request_line;
    this->method = parts[0];
    this->uri = parts[1];
    this->http_version = parts[2];
}

void http_request_t::parse_header_line(const std::string &header_line) {
    if (header_line.empty()) {
        return;
    }

    const auto result = boost::find_first(header_line,
                                          http_constants_t::HEADER_DELIMITER);
    if (!result) {
        throw std::runtime_error("ヘッダのデリミタ \":\" が含まれていません");
    }

    const std::string key(header_line.begin(), result.begin());
    const std::string value(result.end(), header_line.end());

    if (this->header.count(key) == 0) {
        this->header[key] = value;
    } else {
        std::cerr << "ヘッダのキーが重複しているため無視されました。 "
                  << "key=" << key
                  << std::endl;
    }
}

bool http_request_t::is_header_ready() const {
    // メソッド が読み込み済みの場合は、ヘッダ全体も読み込み済みとするtosuru
    return !this->method.empty();
}

bool http_request_t::is_body_ready() const {
    // Body がない場合は常に Ready とする
    if (this->content_length == 0) {
        return true;
    }

    // Body を Content-Length で指定されたバイト長まで読み込んでいれば Ready とする
    return this->body.size() >= this->content_length;
}
