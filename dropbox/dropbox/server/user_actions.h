#ifndef USERACTIONS_H
#define USERACTIONS_H

#include "../lib/netdisk.h"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <sys/io.h>

class UserActions
{

public:
    UserActions(int connfd, SSL *ssl);
    ~UserActions();
    void RecvFile();
    void SendFile();
    void GetDirinfo();
    int ReadMsg();
    void DeleteFile();
    void Login();
    void Register();
    bool CheckCookie(const char *recv_buff);
    void DeleteAccount();

private:
    int conn_fd;
    SSL *ssl;
    struct netmsg_header client_initheader;
    std::string user_path = "/usr1/";
    std::string base_path = "./userspace/";
    char *send_buff;
    char *recv_buff;
    char *file_buff;
    std::string cookie;
    std::string current_username;
};

#endif