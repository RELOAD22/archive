#ifndef NETDISK_H
#define NETDISK_H
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glog/logging.h>
#include "json.hpp"
//#include "jsoncpp/json/json.h"
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define UNDEFINE_TYPE 0
#define REGISTER_REQUEST 1
#define REGISTER_RESPONSE 2
#define REGISTER_DELETE 5
#define LOGIN_REQUEST 11
#define LOGIN_RESPONSE 12
#define CATALOG_REQUEST 21
#define CATALOG_RESPONSE 22
#define FILE_REQUEST 31
#define FILE_UPLOAD 32
#define FILE_DELETE 33
#define FILE_METADATA 34
#define FILE_CONTENT 35
#define TCP_KEEPALIVE 41


#define PUBLIC_PORT 1777
#define MAX_BUFF 4096

#define MSG_HEADERSIZE sizeof(netmsg_header)
#define MSG_FILEINFOSIZE sizeof(netmsg_fileinfo)

struct netmsg_header
{
    uint32_t type = 0;
    uint32_t size = 0;
};

struct netmsg_fileinfo
{
    uint32_t filesize;
    char filename[32];
    char filepath[256];
};

#endif