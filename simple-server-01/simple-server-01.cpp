//
// Created by munenaga on 2020/01/25.
//

#include "common.h"
#include "http_request_t.h"
#include "http_response_t.h"
#include "http_server_t.h"
#include <sys/socket.h>

#include <vector>
#include <optional>
#include <boost/algorithm/string/join.hpp>
#include <http_constants_t.h>

/**
 * HTTP 用のソケットを処理する
 *
 * 今回の simple-server-01 では リクエストを読み込んで、応答を返すだけにする
 *
 * @param [in] sd ソケットディスクリプタ
 * @param [in] client_addr クライアントアドレス
 */
void in_process_process_http_socket(int sd, const char* client_addr);

/**
 * レスポンスを書き込む
 * @param [in] sd クライアントとの通信用ソケット
 * @param [in] response 書き込むレスポンス
 */
void write_response(int sd, const http_response_t &response);


/**
 * simple-server-01 のエントリポイント
 *
 * simple-server-01 は とても単純な HTTP サーバーの例。
 *
 * socket、bind、listen、accept, read/write
 * がお決まりのパターンなので一通り示す。
 *
 * @return 終了コード
 */
int main() {
    http_server_t server;
    server.set_client_handler(
        in_process_process_http_socket
    );
    server.start(
        nullptr,
        12345
    );
}


void in_process_process_http_socket(int sd, const char* client_addr) {
    const auto request = http_server_t::read_request(sd);

    if (request) {
        // 単純にエコーする
        http_response_t response;
        response.set_status(200);
        response.add_header("Content-Type", "text/plain;charset=UTF-8");

        std::vector<std::string> response_lines;
        response_lines.push_back(request->get_request_line());

        for (const auto &header : request->get_header()) {
            response_lines.push_back(
                header.first
                + http_constants_t::HEADER_DELIMITER
                + header.second);
        }
        response.set_body(
            boost::join(response_lines, http_constants_t::CRLF)
                .append(http_constants_t::CRLF)
                .append(request->get_body())
                .append(http_constants_t::CRLF)
        );

        write_response(sd, response);
    }

    shutdown(sd, SHUT_RDWR);
    close(sd);
}

void write_response(int sd, const http_response_t &response) {
    const std::string response_text = response.to_string();
    std::vector<char> buffer(response_text.begin(), response_text.end());
    auto remaining_size = buffer.size();

    while (remaining_size > 0) {
        auto current_size = ::send(sd, (&*response_text.begin()) + (response_text.size() - remaining_size), remaining_size, MSG_DONTWAIT);
        if (static_cast<int>(current_size) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            if (errno == EINTR) {
                if (http_server_t::is_shutdown_required()) {
                    return;
                }
                continue;
            }
            http_server_t::print_error(errno);
            return;
        }
        remaining_size -= current_size;
    }

}

