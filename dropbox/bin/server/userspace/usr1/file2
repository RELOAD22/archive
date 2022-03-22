#ifndef TCPSERVER_H
#define TCPSERVER_H
#include "../lib/netdisk.h"
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
#include "jsoncpp/json/json.h"

using namespace std;

namespace tssr
{

#define MAX_CLIENT 10

    enum
    {
        TSS_LISTENNING,
        TSS_ACCEPTING
    };

    struct conn_info
    {
        int socket = -1;
        string ip = "";
        int id = -1;
    };

    class TcpServer
    {

    public:
        TcpServer();
        int Setup(int port, vector<int> opts = vector<int>());
        void Accept();

        void Close();

    private:
        static void *Handler(void *arg);
        int listen_fd;
        int num_connclient;
        int server_status;

        struct sockaddr_in server_addr_;
        struct sockaddr_in client_addr_;
    };

}
#endif