#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/io.h>
#include <dirent.h>
#include "tcpserver.h"
#include "../lib/netdisk.h"

using namespace tssr;

int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]); // 初始化GLog库

    std::string log_dir = "./log/";
    if (access(log_dir.c_str(), 0) == -1)                                        //如果文件夹不存在
        mkdir(log_dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO); //则创建

    FLAGS_log_dir = log_dir; // 将日志文件输出到本地
    FLAGS_alsologtostderr = 1;
    LOG(INFO) << "LOG system start";

    TcpServer ts1;
    ts1.Setup(PUBLIC_PORT);
    ts1.Accept();
    ts1.Close();
    return 0;
}