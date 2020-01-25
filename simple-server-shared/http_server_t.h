//
// Created by munenaga on 2020/01/25.
//

#ifndef HTTP_SERVER_HTTP_SERVER_T_H
#define HTTP_SERVER_HTTP_SERVER_T_H

class http_request_t;

/**
 * HTTPサーバークラス
 */
class http_server_t {
public:
    /**
     * サーバーの待ち受けを開始する
     *
     * @param [in] ip_address リッスンするIPアドレス。 nullptr が指定された場合は全て
     * @param [in] port リッスンするポート 必須
     */
    void start(
        const char* ip_address,
        ushort port
    );

    /**
     * エラーを出力する
     * @param _errno エラー番号
     */
    static void print_error(int _errno);

    /**
     * シャットダウンが要求されたか?
     * @return シャットダウンが要求された場合 `true`.
     */
    inline static bool is_shutdown_required() {
        return http_server_t::shutdown_required;
    }


    /**
     * クライアントソケット処理ハンドラをセットする。
     *
     * ※ クライアントから接続があった場合、アクセプトに成功したら呼ばれるハンドラ
     *
     * ※ この中でソケットの `shutdown` と `close` も行ってください。
     *
     * @param [in] client_handler ソケット処理ハンドラ
     */
    void set_client_handler(std::function<void(int, const char*)> client_handler);

    /**
     * ソケットからリクエストを読み込む
     * @param [in] sd ソケットディスクリプタ
     * @return リクエスト
     */
    static std::shared_ptr<http_request_t> read_request(int sd);

private:
    /**
     * クライアントソケット処理ハンドラ
     */
    std::function<void(int, const char*)> client_handler;

    /**
     * 接続されたクライアントを処理する
     * @param [in] sd ソケットディスクリプタ
     * @param [in] client_addr クライアントのアドレス
     */
    void handle_client(int sd, const sockaddr_in* client_addr);

    /**
     * シャットダウン要求フラグ
     */
    static volatile bool shutdown_required;

    /**
     * シグナルハンドラ登録フラグ
     */
    static bool signal_handlers_registered;

    /**
     * シグナルハンドラを登録する
     */
    static void register_signal_handlers();

    /**
     * シグナルハンドラ (SIGINT的ななシグナルをトラップして、サーバーを停止するため)
     * @param [in] signum シグナル番号
     */
    static void signal_handler(int signum);
};


#endif //HTTP_SERVER_HTTP_SERVER_T_H
