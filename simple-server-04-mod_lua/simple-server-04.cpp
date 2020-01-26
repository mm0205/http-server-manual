//
// Created by munenaga on 2020/01/26.
//

#include "common.h"
#include <http_server_t.h>
#include <http_request_t.h>
#include <http_response_t.h>

extern "C" {
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lualib.h>
}

/**
 * 別スレッドで lua を実行して、レスポンスを `sd` に `send` する
 *
 * ※ 手抜き実装なので、スレッドの終了待ちは行わない。
 *
 *
 * @param [in] sd クライアントとの通信用ソケットディスクリプタ
 * @param [in] client_ip クライアントのIPアドレス
 */
void run_lua(int sd, const char* client_ip);

void run_lua_impl(int sd, std::string client_ip);

void write_response(int sd, const http_response_t &response);

std::vector<std::thread> g_threads;

/**
 * lua を サーバープロセスに取り込んで、サーバー側の子スレッドでインタープリットする
 *
 * @return
 */
int main() {
    http_server_t server;
    server.set_client_handler(
        run_lua
    );

    server.start(
        nullptr,
        12348
    );
}

void run_lua(int sd, const char* client_ip) {
    // C++ はコールバックで渡されたポインタを、この関数を抜けても使いたい場合は、
    // コピーしておきましょう。
    std::string client_ip_text(client_ip);

    // スレッドを起動する
    std::thread new_thread([sd, client_ip_text] {
        run_lua_impl(sd, client_ip_text);
    });


    g_threads.push_back(std::move(new_thread));
}

void run_lua_impl(int sd, std::string client_ip) { // NOLINT(performance-unnecessary-value-param)

    const auto request = http_server_t::read_request(sd);

    // lua の環境
    lua_State* L = luaL_newstate();
    if (!L) {
        std::cerr << "Lua の初期化に失敗しました。" << std::endl;
    }
    // 標準ライブラリをロードする
    luaL_openlibs(L);

    // ファイルをロードする
    luaL_loadfile(L, "/Users/munenaga/projects/mm0205/http-server/cgi/request_handler.lua");

    // ファイルをロードして実行すると、ファイル内の関数定義などが行われる
    lua_call(L, 0, 0);


    // lua スクリプト内の リクエストハンドラをスタック先頭に積む
    // (後でコールするので)
    lua_getglobal(L, "request_handler");


    // リクエストを渡すようにテーブルを作成する
    lua_newtable(L);

    // リクエスト行をテーブルにセット
    lua_pushstring(L, "requestLine");
    lua_pushstring(L, request->get_request_line().c_str());
    lua_settable(L, -3);

    const auto &header = request->get_header();
    for (const auto &header_item : header) {
        lua_pushstring(L, header_item.first.c_str());
        lua_pushstring(L, header_item.second.c_str());
        lua_settable(L, -3);
    }

    // リクエストボディもテーブルにセット
    lua_pushstring(L, "body");
    lua_pushstring(L, request->get_body().c_str());
    lua_settable(L, -3);

    // 今、スタックには
    // * 引数のテーブル
    // * request_handler
    // という感じで積まれている
    // この関数を実行する
    // `lua_call` の引数は Lua関数の引数の数:1, 戻り値の数が1 という意味
    lua_call(L, 1, 1);

    const char* response_ptr = lua_tostring(L, -1);

    const std::string response_text(response_ptr);

    // lua の環境を閉じる
    lua_close(L);

    http_response_t response;
    response.add_header("Content-Type", "text/plain");
    response.set_body(response_text);

    write_response(sd, response);

    // ソケットのクローズ
    close(sd);
}

void write_response(int sd, const http_response_t &response) {
    const std::string response_text = response.to_string();
    std::vector<char> buffer(response_text.begin(), response_text.end());
    auto remaining_size = buffer.size();

    while (remaining_size > 0) {
        auto current_size = ::send(sd, (&*response_text.begin()) + (response_text.size() - remaining_size),
                                   remaining_size, MSG_DONTWAIT);
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

