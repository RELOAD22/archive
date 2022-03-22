#include "tcpserver.h"
#include <dirent.h>
#include <fstream>
#include <vector>
#include <iostream>
#include "user_actions.h"

using namespace tssr;
using namespace std;

TcpServer::TcpServer()
{
}
SSL_CTX *ctx;

void SetTimeout_recv(int so, int sec)
{
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 0;
    setsockopt(so, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
}

void *TcpServer::Handler(void *arg)
{
    pthread_detach(pthread_self());
    int conn_fd = *((int *)arg);

    SSL *ssl;
    /* 基于 ctx 产生一个新的 SSL */
    ssl = SSL_new(ctx);
    /* 将连接用户的 socket 加入到 SSL */
    SSL_set_fd(ssl, conn_fd);
    /* 建立 SSL 连接 */
    if (SSL_accept(ssl) == -1)
    {
        close(conn_fd);
        return 0;
    }
    UserActions ua(conn_fd, ssl);
    while (ua.ReadMsg())
        ;

    /* 关闭 SSL 连接 */
    SSL_shutdown(ssl);
    /* 释放 SSL */
    SSL_free(ssl);
    /* 关闭 socket */
    close(conn_fd);
    delete (int *)arg;
    LOG(INFO) << "release connection";
    pthread_exit(NULL);
    return 0;
}

int TcpServer::Setup(int port, vector<int> opts)
{

    /* SSL 库初始化 */
    SSL_library_init();
    /* 载入所有 SSL 算法 */
    OpenSSL_add_all_algorithms();
    /* 载入所有 SSL 错误消息 */
    SSL_load_error_strings();
    /* 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text */
    ctx = SSL_CTX_new(SSLv23_server_method());
    /* 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准 */
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 载入用户的数字证书， 此证书用来发送给客户端。 证书里包含有公钥 */
    if (SSL_CTX_use_certificate_file(ctx, "cacert.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 载入用户私钥 */
    if (SSL_CTX_use_PrivateKey_file(ctx, "privkey.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 检查用户私钥是否正确 */
    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        PLOG(ERROR) << "create listen socket fd";

    bzero(&server_addr_, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr_.sin_port = htons(port);

    if (bind(listen_fd, (const sockaddr *)&server_addr_, sizeof(server_addr_)) < 0)
        PLOG(ERROR) << "bind listen socket fd";

    if (listen(listen_fd, MAX_CLIENT) < 0)
        PLOG(ERROR) << "listen socket fd";

    num_connclient = 0;
    server_status = TSS_LISTENNING;
    return 0;
}

void TcpServer::Close()
{
    if (!close(listen_fd))
        PLOG(ERROR) << "close listen socket fd";
}

void TcpServer::Accept()
{
    auto addr_size = sizeof(client_addr_);
    for (;;)
    {
        int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr_, (socklen_t *)&addr_size);
        pthread_t *server_thread = new pthread_t;
        int *conn_fd_ptr = new int;
        *conn_fd_ptr = conn_fd;
        LOG(INFO) << "accept new connection";
        pthread_create(server_thread, NULL, &Handler, (void *)conn_fd_ptr);
    }
}
