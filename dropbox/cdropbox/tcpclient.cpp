#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <iostream>
#include <vector>
#include "lib/netdisk.h"
#include "lib/json.hpp"
using json = nlohmann::json;
//#include "jsoncpp/json/json.h"

using namespace std;

char recv_buff[MAX_BUFF];
char send_buff[MAX_BUFF];
char file_buff[MAX_BUFF];
SSL_CTX *ctx;
SSL *ssl;
int connfd;
string cookie = "";

int BuildJsonMsg(char *send_buff, int msg_type, json j)
{
    // cookie
    if (cookie != "")
    {
        j["cookie"] = cookie;
    }

    string json_string = j.dump();
    const char *json_str = json_string.c_str();
    size_t json_size = strlen(json_str) + 1;

    struct netmsg_header send_header;
    send_header.type = msg_type;
    send_header.size = json_size;

    int total_size = MSG_HEADERSIZE + json_size;
    memset(send_buff, 0, total_size);
    memcpy(send_buff, (void *)&send_header, MSG_HEADERSIZE);
    memcpy((char *)send_buff + MSG_HEADERSIZE, (void *)json_str, json_size);
    return total_size;
}

// parse json value from recv msg buff
/*
Json::Value ParseJsonMsg(const char * json_msg){
    Json::Value root;
    Json::Reader reader;
    reader.parse(json_msg + MSG_HEADERSIZE, root);
    return root;
}*/

void ShowCerts(SSL *ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL)
    {
        printf("数字证书信息:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("证书: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("颁发者: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("无证书信息！\n");
}

void SendFile(string path)
{
    printf("upload file: path: %s\n", path.c_str());
    json sendfile_json;

    // get file info
    int pos = path.rfind('/');
    string filename = path.substr(pos + 1, path.length());
    FILE *fq = fopen(path.c_str(), "rb");
    if (!fq)
    {
        printf("open file error\n");
        printf("file error: %s(errno:%d)\n", strerror(errno), errno);
        return;
    }
    struct stat statbuf;
    stat(path.c_str(), &statbuf);
    uint32_t filesize = statbuf.st_size;

    // build msg
    sendfile_json["filename"] = filename;
    sendfile_json["filesize"] = filesize;
    int total_size = BuildJsonMsg(send_buff, FILE_UPLOAD, sendfile_json);

    // send file info
    SSL_write(ssl, send_buff, total_size);

    // send file content
    while (!feof(fq))
    {
        sleep(1);
        int len = fread(file_buff, 1, MAX_BUFF, fq);
        SSL_write(ssl, file_buff, len);
    }
    fclose(fq);
    printf("send finish\n");
}

void DownloadFile(string path)
{
    json download_json;
    download_json["filepath"] = path;
    int total_size = BuildJsonMsg(send_buff, FILE_REQUEST, download_json);

    if (SSL_write(ssl, send_buff, total_size) < 0)
    {
        printf("send socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    if (SSL_read(ssl, recv_buff, MAX_BUFF) < 0)
    {
        printf("recv socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    json fileinfo_json = json::parse(recv_buff + MSG_HEADERSIZE);
    int filesize = fileinfo_json["filesize"];

    int pos = path.rfind('/');
    string filename = path.substr(pos + 1, path.length());

    FILE *fq = fopen(filename.c_str(), "wb");
    cout << filename << endl;
    if (!fq)
    {
        printf("open file error\n");
        printf("file error: %s(errno:%d)\n", strerror(errno), errno);
        exit(1);
    }
    int totalsize = 0;
    while (1)
    {
        int len = SSL_read(ssl, file_buff, MAX_BUFF);
        fwrite(file_buff, 1, len, fq);
        totalsize += len;
        if (totalsize >= filesize)
            break;
    }

    fclose(fq);
    printf("recv finish\n");
}

void DeleteFile(string path)
{
    json deletefile_json;
    deletefile_json["deletepath"] = path;
    int total_size = BuildJsonMsg(send_buff, FILE_DELETE, deletefile_json);

    if (SSL_write(ssl, send_buff, total_size) < 0)
    {
        printf("send socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    printf("delete finish\n");
}

vector<string> OpenDir()
{

    json root;
    root["opendir"] = "/";
    int total_size = BuildJsonMsg(send_buff, CATALOG_REQUEST, root);

    if (SSL_write(ssl, send_buff, total_size) < 0)
    {
        printf("send socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    if ((SSL_read(ssl, recv_buff, MAX_BUFF)) < 0)
    {
        printf("recv socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    auto fileinfo_json = json::parse(recv_buff + MSG_HEADERSIZE);
    cout << "opendir: " << fileinfo_json << endl;
    vector<string> res_vec = fileinfo_json["files"];
    return res_vec;
}

bool Login(string username, string passwd)
{
    json logininfo;
    logininfo["username"] = username;
    logininfo["password"] = passwd;
    int total_size = BuildJsonMsg(send_buff, LOGIN_REQUEST, logininfo);

    if (SSL_write(ssl, send_buff, total_size) < 0)
    {
        printf("send socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    if ((SSL_read(ssl, recv_buff, MAX_BUFF)) < 0)
    {
        printf("recv socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    auto loginresp = json::parse(recv_buff + MSG_HEADERSIZE);
    cout << "loginresp: " << loginresp << endl;
    string status = loginresp["status"];

    if (status == string("success"))
    {
        cookie = loginresp["set-cookie"];
        return true;
    }
    else
        return false;
}

bool Register(string username, string passwd)
{
    json regisinfo;
    regisinfo["username"] = username;
    regisinfo["password"] = passwd;
    int total_size = BuildJsonMsg(send_buff, REGISTER_REQUEST, regisinfo);

    if (SSL_write(ssl, send_buff, total_size) < 0)
    {
        printf("send socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    if ((SSL_read(ssl, recv_buff, MAX_BUFF)) < 0)
    {
        printf("recv socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    auto regisresp = json::parse(recv_buff + MSG_HEADERSIZE);
    cout << "regisresp: " << regisresp << endl;
    string status = regisresp["status"];
    if (status == string("success"))
    {
        return true;
    }
    else
        return false;
}

void DeleteAccount()
{
    json regisinfo;
    int total_size = BuildJsonMsg(send_buff, REGISTER_DELETE, regisinfo);

    if (SSL_write(ssl, send_buff, total_size) < 0)
    {
        printf("send socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
}

void KeepAlive()
{
    json keepinfo;
    int total_size = BuildJsonMsg(send_buff, TCP_KEEPALIVE, keepinfo);

    if (SSL_write(ssl, send_buff, total_size) < 0)
    {
        printf("send socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
}

int tcpclient_init()
{
    struct sockaddr_in conn_sock;

    const char *server_addr = "127.0.0.1";
    if ((connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("creat socket fd error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }

    bzero(&conn_sock, sizeof(conn_sock));
    conn_sock.sin_family = AF_INET;
    conn_sock.sin_port = htons(PUBLIC_PORT);
    inet_pton(AF_INET, server_addr, &conn_sock.sin_addr.s_addr);

    /* SSL 库初始化，参看 ssl-server.c 代码 */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    if (connect(connfd, (const sockaddr *)&conn_sock, sizeof(conn_sock)) < 0)
    {
        printf("connect socket error: %s(errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    sleep(1);

    /* 基于 ctx 产生一个新的 SSL */
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, connfd);
    /* 建立 SSL 连接 */
    if (SSL_connect(ssl) == -1)
        ERR_print_errors_fp(stderr);
    else
    {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);
    }
    return 1;
}

int tcpclient_release()
{
    /* 关闭连接 */
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(connfd);
    SSL_CTX_free(ctx);
    printf("tcpclient release!\n");
    return 1;
}
