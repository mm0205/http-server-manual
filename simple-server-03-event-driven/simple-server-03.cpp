//
// Created by munenaga on 2020/01/26.
//

#include "common.h"

#include <boost/asio.hpp>
#include <http_response_t.h>
#include "http_request_t.h"

/**
 * boost::ip::tcp::socket が ムーブコンストラクタしか持ってないので、ホルダを用意して管理する
 */
class socket_holder_t {
public:
    inline boost::asio::ip::tcp::socket &get_socket() {
        return _socket;
    }

    socket_holder_t(boost::asio::ip::tcp::socket &&socket)
        : _socket(std::move(socket)) {}

private:
    boost::asio::ip::tcp::socket _socket;
};

void do_signal_handler_async(
    boost::asio::signal_set &signals,
    boost::asio::ip::tcp::acceptor &acceptor,
    std::vector<std::shared_ptr<socket_holder_t>> &sockets
);

void process_request(std::vector<std::shared_ptr<socket_holder_t>> &sockets, boost::asio::ip::tcp::socket &socket);

void process_request_with_buffer(
    std::vector<std::shared_ptr<socket_holder_t>> &sockets, boost::asio::ip::tcp::socket &socket,
    std::shared_ptr<std::array<char, 1024>> buffer,
    std::shared_ptr<http_request_t> request
);

void write_response(boost::asio::ip::tcp::socket &socket, std::shared_ptr<http_response_t> response);

void do_accept(boost::asio::ip::tcp::acceptor& acceptor,
    std::vector<std::shared_ptr<socket_holder_t>> &sockets);

/**
 * Boost.Asio を使って非同期I/O HTTPサーバーを構築する
 * @return
 */
int main() {

    // ソケット
    std::vector<std::shared_ptr<socket_holder_t>> sockets;

    /* #####################################################################
     * IOコンテキストの準備
     * ##################################################################### */
    // IOコンテキスト
    // 非同期I/O を使う場合のI/Oコンテキスト
    // Asio 系の APIを使うときに渡しておくのに必要
    boost::asio::io_context _io_context(1);
    // Acceptor (後述)
    boost::asio::ip::tcp::acceptor _acceptor(_io_context);

    /* #####################################################################
     * シグナルハンドラの登録処理 (Boostは便利)
     * ##################################################################### */

    // シグナル処理も Asio がハンドラの登録とかやってくれる
    boost::asio::signal_set _signals(_io_context);
    // 終了系のシグナルを登録しておいて、
    _signals.add(SIGTERM);
    _signals.add(SIGINT);
    _signals.add(SIGQUIT);
    // シグナルが届くまでまって、届いたら終了処理をするハンドラを実行する
    do_signal_handler_async(
        _signals,
        _acceptor,
        sockets
    );

    /* #####################################################################
     * Acceptor を作ります。
     * Asio での accept() するソケットのことです。
     * ##################################################################### */
    boost::asio::ip::tcp::resolver resolver(_io_context);
    auto end_point = *resolver.resolve(
        boost::asio::ip::tcp::v4(), // IPv4 を指定
        "",                   // IPアドレスを指定 (システムにお任せ)
        "12347",            // ポートを指定
        boost::asio::ip::tcp::resolver::passive // passive (コネクションを受け付ける用ですよ。と指定)
    ).begin();


    _acceptor.open(end_point.endpoint().protocol());
    _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor.bind(end_point);
    _acceptor.listen();
    std::cout << "address: " << end_point.endpoint().address().to_string() << std::endl;

    /* #####################################################################
     * 待ち受けを開始します。
     * クライアントから接続があると引数のラムダが実行されます。
     * ##################################################################### */

    do_accept(_acceptor, sockets);

    _io_context.run();

    return 0;
}

void do_accept(boost::asio::ip::tcp::acceptor& _acceptor, std::vector<std::shared_ptr<socket_holder_t>> &sockets) {
    _acceptor.async_accept(
        [&_acceptor, &sockets](boost::system::error_code error_code, boost::asio::ip::tcp::socket socket) {
            std::cerr << error_code << std::endl;
            auto holder = std::make_shared<socket_holder_t>(std::move(socket));
            sockets.push_back(holder);
            process_request(sockets, holder->get_socket());
            do_accept(_acceptor, sockets);
        }
    );
}

void process_request(std::vector<std::shared_ptr<socket_holder_t>> &sockets, boost::asio::ip::tcp::socket &socket) {
    auto buffer = std::make_shared<std::array<char, 1024>>();
    auto request = std::make_shared<http_request_t>();

    process_request_with_buffer(sockets, socket, buffer, request);
}

void process_request_with_buffer(
    std::vector<std::shared_ptr<socket_holder_t>> &sockets, boost::asio::ip::tcp::socket &socket,
    std::shared_ptr<std::array<char, 1024>> buffer,
    std::shared_ptr<http_request_t> request
) {
    socket.async_read_some(boost::asio::buffer(*buffer), [&sockets, &socket, buffer, request](
        boost::system::error_code ec, std::size_t bytes_transferred
    ) {
        std::cout << "called" << std::endl;
        if (ec) {
            socket.close();
            return;
        }
        if (bytes_transferred == 0) {
            process_request_with_buffer(sockets, socket, buffer, request);
            return;
        }
        std::string current_text(buffer->data(), buffer->data() + bytes_transferred);
        request->add_bytes(current_text);
        if (!request->is_ready()) {
            process_request_with_buffer(sockets, socket, buffer, request);
            return;
        }

        auto response = std::make_shared<http_response_t>();
        response->add_header("Content-Type", "text/plain");
        response->set_body("受信した内容\r\n" + request->get_body());

        write_response(socket, response);
    });
}

void write_response(boost::asio::ip::tcp::socket &socket, std::shared_ptr<http_response_t> response) {
    // バッファに書き込む
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(
        boost::asio::buffer(response->to_string())
    );
    // 書き込みが終わると、ラムダが呼ばれる
    boost::asio::async_write(
        socket,
        buffers,
        [&socket](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                boost::system::error_code ignored_ec;
                socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                                ignored_ec);
            }

            if (ec != boost::asio::error::operation_aborted) {
                socket.close();
            }
        });
}

void do_signal_handler_async(
    boost::asio::signal_set &_signals,
    boost::asio::ip::tcp::acceptor &acceptor,
    std::vector<std::shared_ptr<socket_holder_t>> &sockets
) {

    // _signals に登録されたシグナルを受信したら、
    // 引数のラムダを実行してくれる!
    _signals.async_wait([&acceptor, &sockets](boost::system::error_code /*ec*/, int /*signum*/) {
        // 待ち受けをやめる
        acceptor.close();

        // 既存の接続を全部クローズ
        for (auto &socket: sockets) {
            if (socket->get_socket().is_open()) {
                socket->get_socket().close();
            }
        }
    });
}
