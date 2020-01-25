//
// Created by munenaga on 2020/01/25.
//

#include "common.h"
#include <strings.h>
#include <sys/fcntl.h>


#include "http_server_t.h"
#include "http_request_t.h"

bool http_server_t::signal_handlers_registered = false;
volatile bool http_server_t::shutdown_required = false;

void http_server_t::start(
    const char* ip_address,
    ushort port
) {
    if (!signal_handlers_registered) {
        register_signal_handlers();
        signal_handlers_registered = true;
    }

    /* #####################################################################
       * 待ち受けるIPアドレスにソケットをバインドする
       * ##################################################################### */
    // ソケットを作成する
    //
    // 第一引数:
    //  AF_INET は普通のIPV4ソケット
    //  他に普通に使う定数としては以下がある
    //      * AF_INET6 => IPv6 ソケット
    //      * AF_UNIX => ユニックスソケット (ドキュメントで別途説明する)
    //
    // 第二引数:
    //  SOCK_STREAM は TCP通信するときにしている
    //  他に使うことがあるのは、以下の通り
    //      * SOCK_DGRAM => UDPソケットに使う
    //      * SOCK_RAW => 名前のネットワークパケットにアクセスするときに使う
    //                  組み込み以外で実用したことはない (自前でping (ICMP) を送受信するなど)。
    //                  アンチウィルスソフト とか作るときはもしかしたら使うのかも。。。
    //
    //  SOCK_CLOEXEC
    //      => これを指定すると、forkするときに子プロセスからソケットディスクリプタが見えなくなる
    //          のだが、 どうも macos にはないらしい。。。
    //          Linux で マルチスレッドでソケット使うときは とりあえず指定しておけば良いかも。
    //          もともとLinux用。今時はBSDでも実装しているはずなんだけど、
    //          macOS は BSD4 由来だからか？
    //
    //  SOCK_NONBLOCK
    //      => これを指定すると、ソケットディスクリプタで O_NONBLOCK がONになる。
    //          O_NONBLOCK
    //          のだが、やはり mac にはない。
    //          MANページを読んでいると、どうもPOSIX は NOBLOCK を使うよりも select で無限待ちをする方を推奨しているように思える。
    //
    auto sd = socket(
        AF_INET,
        SOCK_STREAM /* | SOCK_CLOEXEC | SOCK_NONBLOCK*/,
        0
    );

    if (sd == -1) {
        print_error(errno);
        return;
    }

    // fork したときに 親のSDハクローズする
    auto fd_flags = fcntl(sd, F_GETFD);
    fd_flags |= FD_CLOEXEC; // NOLINT(hicpp-signed-bitwise)
    fcntl(sd, F_SETFD, fd_flags);

    /* #####################################################################
     * 待ち受けるIPアドレスにソケットをバインドする
     * ##################################################################### */

    // これはIPv4 アドレス
    struct sockaddr_in addr{};
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    // これはポート番号
    addr.sin_port = htons(static_cast<ushort>(port)); // NOLINT(hicpp-signed-bitwise)

    if (ip_address) {
        struct in_addr ip_addr{};
        inet_aton(ip_address, &ip_addr);
        addr.sin_addr = ip_addr;
    } else {
        // 全ての利用可能なIPアドレスで待ち受ける場合
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    // バインドする
    auto ret = bind(sd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret == -1) {
        print_error(errno);
        close(sd);
    }

    /* #####################################################################
     * クライアントからの接続を待ち受ける
     *
     * 第二引数の 10 は 接続待ちキューの最大数です。
     * 次の accept() を呼ぶまでの間に 11 以上のクライアントがアクセスしてくると、
     * クライアントは、ECONNREFUSED を受け取ります。
     * ##################################################################### */
    ret = listen(sd, 10);
    if (ret == -1) {
        print_error(errno);
    }

    /* #####################################################################
     * クライアントからの接続を受け付ける
     * ##################################################################### */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    for (;;) {
        struct sockaddr_in client_addr{};
        auto client_addr_size = sizeof(client_addr);
        bzero(&client_addr, client_addr_size);

        auto client_sd = accept(sd,
                                reinterpret_cast<sockaddr*>(&client_addr),
                                reinterpret_cast<socklen_t*>(&client_addr_size)
        );

        if (client_sd > 0) {
            this->handle_client(client_sd, &client_addr);
        } else if (errno != EINTR) {
            // EINTR は シグナル受信による中断を示す。
            // このプログラムはシグナルを受け取る予定がないので、エラー扱いにしても良いのだが、
            // デバッグ時にブレークポイントを設定すると、SIGNALが発生するようなので、リトライする。
            //
            // 本番用のコードでは、大体シグナルハンドラでグローバルなフラグをセットする、
            // accept等 (read/writeとかもEINTRを返す) では EINTRが帰ってきたら自分に関係のある フラグがセットされているか確認して、
            // 無関係ならば、リトライし、何か意味のあるフラグがONになっていたらそれようの処理を行う(ループを抜けるとか。。。)
            // 的な実装になる。
            //
            print_error(errno);
            return;
        }
        // 終了系のシグナルが送られた場合はソケットを閉じて終了する
        if (http_server_t::shutdown_required) {
            shutdown(client_sd, SHUT_RDWR);
            close(sd);
            break;
        }
    }
#pragma clang diagnostic pop
}

void http_server_t::print_error(int _errno) {
    std::vector<char> buffer(1024, '\0');
    strerror_r(_errno, &*buffer.begin(), buffer.size());
    std::cerr << "error: "
              << _errno
              << " " << std::string(&*buffer.begin())
              << std::endl;
}

void http_server_t::register_signal_handlers() {

    struct sigaction my_action{};

    sigemptyset(&my_action.sa_mask);
    my_action.sa_flags = 0;
    my_action.sa_handler = signal_handler;

    struct sigaction temp_action{};
    int target_signals[] = {
        SIGINT,
        SIGHUP,
        SIGTERM
    };

    for (auto signal_number : target_signals) {
        auto ret = sigaction(signal_number, nullptr, &temp_action);
        if (ret == -1) {
            print_error(errno);
            exit(999);
        }

        if (temp_action.sa_handler != SIG_IGN) {
            sigaction(signal_number, &my_action, nullptr);
        }
    }
}

void http_server_t::signal_handler(int signum) {
    http_server_t::shutdown_required = true;
}

void http_server_t::set_client_handler(std::function<void(int, const char*)> client_handler) {
    this->client_handler = client_handler;
}

void http_server_t::handle_client(int sd, const sockaddr_in* client_addr) {

    // クライアントのIPアドレスを文字列形式で保持する。
    std::vector<char> client_address_buffer(INET_ADDRSTRLEN + 1, '\0');
    ::inet_ntop(AF_INET, &client_addr->sin_addr, &*client_address_buffer.begin(), INET_ADDRSTRLEN);
    const std::string client_ip(&*client_address_buffer.begin());

    // ハンドラが登録されていれば処理する
    if (this->client_handler) {
        this->client_handler(sd, client_ip.c_str());
    }
}

std::shared_ptr<http_request_t> http_server_t::read_request(int sd) {
    const size_t TEMP_BUFFER_SIZE = 1024;
    const char BUFFER_INITIAL_VALUE = '\0';

    // HTTPリクエスト
    auto request = std::make_shared<http_request_t>();

    // 一時バッファ
    std::vector<char> temp_buffer(TEMP_BUFFER_SIZE, BUFFER_INITIAL_VALUE);

    /* #####################################################################
     * クライアントからのリクエストを受信する
     *
     * ソケット通信は recv で一度に全部読み込めるとは限らないので、
     * 基本的に、リクエスト全体を読み込むまでループする。
     *
     * ##################################################################### */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    for (;;) {
        // 受信する
        // 今回はノンブロッキングで受信して、
        // データを受信する毎にリクエストの最後まで読み込んだかチェックする
        auto received_size = recv(
            sd,
            &*temp_buffer.begin(),
            temp_buffer.size() - 1,
            MSG_DONTWAIT
        );
        if (static_cast<int>(received_size) == -1) {
            // シグナル割り込みの場合はリトライ
            if (errno == EINTR) {
                if (!http_server_t::is_shutdown_required()) {
                    continue;
                }
            }
            // ブロックされた場合 (まだデータが受信できる状態ではない場合)もリトライ
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            // それ以外のエラーコードの場合はエラーにする
            http_server_t::print_error(errno);
            break;
        }

        // バッファの内容を `string` にする (下のように書くと最初の '\0' までの文字列になる)
        const std::string current_text(&*temp_buffer.begin());

        // 一時バッファをクリアする
        std::vector<char>(TEMP_BUFFER_SIZE, BUFFER_INITIAL_VALUE).swap(temp_buffer);

        // 今回読み込んだ内容をリクエストに追加する
        try {
            request->add_bytes(current_text);
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << std::endl;
            break;
        }

        // リクエストヘッダ全体を受信したか判定する
        if (request->is_ready()) {
            return request;
        }
    }
    return nullptr;
#pragma clang diagnostic pop
}
