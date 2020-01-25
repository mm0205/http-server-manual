//
// Created by munenaga on 2020/01/25.
//

#include "common.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <fcntl.h>
#include <http_response_t.h>

#include "http_request_t.h"
#include "http_server_t.h"

/**
 * CGIを実行する
 * @param [in] sd クライアントとの通信用ソケットディスクリプタ
 * @param [in] client_addr クライアントのIPアドレス
 */
static void run_cgi(int sd, const char* client_addr);

int main() {
    http_server_t server;
    server.set_client_handler(
        run_cgi
    );

    server.start(
        nullptr,
        12346
    );
}


void run_cgi(int sd, const char* client_addr) {
    // CGIとのIO用のファイル
    const auto temp_in = boost::filesystem::unique_path();
    const auto temp_out = boost::filesystem::unique_path();

    std::ofstream out_fs;
    auto out_fd = open(temp_out.native().c_str(), O_CLOEXEC | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); // NOLINT(hicpp-signed-bitwise)

    // CGIの標準出力 にリクエストボディを渡す
    std::ofstream input_fs;

    auto pid = fork();

    if (pid) {

        auto status = 0;
        waitpid(pid, &status, 0);
        // 親プロセスは処理終了

        const auto file_size = boost::filesystem::file_size(temp_out);
        std::cout << file_size << std::endl;
        if (file_size > 0) {
            http_response_t response;

            std::ifstream ifs;
            ifs.open(temp_out.native(), std::ios::in | std::ios::binary);
            std::vector<char> buffer(file_size, '\0');
            ifs.read(&*buffer.begin(), file_size);
            response.set_body(std::string(&*buffer.begin()));
            const auto response_text = response.to_string();
            auto remain_size = response_text.size();

            while(remain_size > 0) {
                auto temp_size = send(sd, (&*response_text.begin()) + (response_text.size() - remain_size), remain_size, MSG_DONTWAIT);
                if (static_cast<int>(temp_size) == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }

                    if (errno == EINTR) {
                        if (http_server_t::is_shutdown_required()) {
                            break;
                        }
                        continue;
                    }
                }
                remain_size -= temp_size;
            }
        }

        close(sd);

        boost::filesystem::remove(temp_in);
        boost::filesystem::remove(temp_out);

        return;
    }

    auto fd_flags = fcntl(sd, F_GETFD);
    fd_flags |= FD_CLOEXEC; // NOLINT(hicpp-signed-bitwise)
    fcntl(sd, F_SETFD, fd_flags);

    // シグナル処理用にダミーでサーバーインスタンスを作っておく
    // HTTPの説明には特に関係ない
    http_server_t server;

    // リクエストを読み込む
    const auto request = http_server_t::read_request(sd);
    input_fs.open(temp_in.native(), std::ios::out | std::ios::binary);
    input_fs << request->get_body();
    input_fs.flush();
    input_fs.close();
    auto in_fd = open(temp_in.native().c_str(), O_CLOEXEC | O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); // NOLINT(hicpp-signed-bitwise)

    // CGIスクリプトに必要な環境変数を設定する
    std::vector<std::string> environment_source;
    environment_source.push_back(
        (boost::format("CONTENT_LENGTH=%d") % request->get_body().size()).str()
    );
    const auto header = request->get_header();
    auto it = header.find("Content-Type");
    if (it != header.end()) {
        environment_source.push_back(
            (boost::format("CONTENT_TYPE=%s") % it->second).str()
        );
    }

    char** env = new char*[environment_source.size() + 1];

    auto i = 0;
    for (const auto &str : environment_source) {
        env[i] = new char[str.length() + 1];
        strcpy(env[i], str.c_str());
        i++;
    }
    env[i] = nullptr;

    char* argv[] = {
        nullptr
    };

    dup2(in_fd, STDIN_FILENO);
    dup2(out_fd, STDOUT_FILENO);

    auto ret = execve("/Users/munenaga/projects/mm0205/http-server/cgi/index.cgi",
           argv,
           env
    );

    // ※ ここに来るのは execve が失敗したときのみ
    // execve は成功すると制御を返さない
    if (ret) {
        http_server_t::print_error(errno);
        shutdown(sd, SHUT_RDWR);
        close(sd);

        for (i = 0; i < environment_source.size(); i++) {
            delete[] env[i];
        }
        delete[] env;

        exit(0);
    }
}