#include <string.h>
#include <vector>
#include "user_actions.h"

using namespace std;
using json = nlohmann::json;

std::string randstr(const int len)
{
	char *str = (char *)malloc(len + 1);
	srand(time(NULL));
	int i;
	for (i = 0; i < len; ++i)
	{
		switch ((rand() % 3))
		{
		case 1:
			str[i] = 'A' + rand() % 26;
			break;
		case 2:
			str[i] = 'a' + rand() % 26;
			break;
		default:
			str[i] = '0' + rand() % 10;
			break;
		}
	}
	str[++i] = '\0';
	std::string res(str);
	free(str);
	return res;
}

void GetFiles(std::string path, std::vector<std::string> &files)
{
	DIR *dir;
	struct dirent *ptr;

	if ((dir = opendir(path.c_str())) == NULL)
	{
		perror("Open dir error...");
		exit(1);
	}

	while ((ptr = readdir(dir)) != NULL)
	{
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		else if (ptr->d_type == 8)
			files.push_back(ptr->d_name);
		// files.push_back(path + ptr->d_name);
		else if (ptr->d_type == 10)
			continue;
		else if (ptr->d_type == 4)
		{
			int pos = strlen(ptr->d_name);
			if (pos < 255)
			{
				ptr->d_name[pos] = '/';
				ptr->d_name[pos + 1] = '\0';
			}
			files.push_back(ptr->d_name);
			// GetFiles(path + ptr->d_name + "/", files);
		}
	}
	closedir(dir);
}

bool UserActions::CheckCookie(const char *recv_buff)
{
	json j = json::parse(recv_buff + MSG_HEADERSIZE);
	if (j.contains("cookie") && j["cookie"] == cookie)
	{
		LOG(INFO) << "right cookie:" << cookie;
		return true;
	}
	else
		return false;
}

int BuildJsonMsg(char *send_buff, int msg_type, json j)
{
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

UserActions::UserActions(int connfd, SSL *inputssl)
{
	ssl = inputssl;
	UserActions::conn_fd = connfd;
	send_buff = (char *)malloc(MAX_BUFF);
	recv_buff = (char *)malloc(MAX_BUFF);
	file_buff = (char *)malloc(MAX_BUFF);
}

UserActions::~UserActions()
{
	free(send_buff);
	free(recv_buff);
	free(file_buff);
}

// RecvFile
// ---response to client's SendFile
void UserActions::RecvFile()
{
	json sendfile_json = json::parse(recv_buff + MSG_HEADERSIZE);

	// set base info
	string filename = sendfile_json["filename"];
	string file_path = base_path + user_path + filename;
	int filesize = sendfile_json["filesize"];

	// open file to write
	FILE *fq = fopen(file_path.c_str(), "wb");
	if (!fq)
	{
		LOG(WARNING) << "open file(wb) error";
		return;
	}
	// recv file content
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
	LOG(INFO) << "new file has been created:" << file_path;
}

// SendFile
// ---response to client's DownloadFile
void UserActions::SendFile()
{
	// get file info from content
	json download_json = json::parse(recv_buff + MSG_HEADERSIZE);

	string file_relapath = download_json["filepath"];
	string file_path = base_path + user_path + file_relapath;
	LOG(INFO) << "sending file: path:" << file_path;

	if (file_relapath.find("../") != string::npos)
	{
		LOG(WARNING) << "permission denied!!";
		return;
	}
	FILE *fq = fopen(file_path.c_str(), "rb");

	// send file metadata to client
	struct stat statbuf;
	stat(file_path.c_str(), &statbuf);
	uint32_t filesize = statbuf.st_size;

	json fileinfo_json;
	fileinfo_json["filesize"] = filesize;
	int total_size = BuildJsonMsg(send_buff, FILE_METADATA, fileinfo_json);

	if (SSL_write(ssl, send_buff, total_size) < 0)
		PLOG(WARNING) << "send socket error";

	// send file content to client
	while (!feof(fq))
	{
		sleep(1);
		int len = fread(file_buff, 1, MAX_BUFF, fq);
		if (len != SSL_write(ssl, file_buff, len))
		{
			PLOG(WARNING) << "send file error";
			return;
		}
	}
	fclose(fq);
	LOG(INFO) << "sending file finish";
}

// DeleteFile
// ---response to client's DeleteFile
void UserActions::DeleteFile()
{
	// get file info from content
	auto deletepath_json = json::parse(recv_buff + MSG_HEADERSIZE);
	LOG(INFO) << "deletepath: " << deletepath_json["deletepath"];

	string file_relapath = deletepath_json["deletepath"];
	string file_path = base_path + user_path + file_relapath;
	LOG(INFO) << "try to delete file:" << file_path;

	if (file_relapath.find("../") != string::npos)
	{
		LOG(WARNING) << "permission denied!!";
		return;
	}
	remove(file_path.c_str());
	LOG(INFO) << "delete file:" << file_path;
}

void UserActions::GetDirinfo()
{
	auto opendir_json = json::parse(recv_buff + MSG_HEADERSIZE);
	LOG(INFO) << "opendir: " << opendir_json["opendir"];

	// open dir, get file infos
	vector<string> filesstr;
	string open_path = base_path + user_path;
	GetFiles(open_path, filesstr);
	string response_filesinfo;

	json fileinfo;
	fileinfo["filenum"] = filesstr.size();
	fileinfo["files"] = filesstr;

	int total_size = BuildJsonMsg(send_buff, CATALOG_RESPONSE, fileinfo);
	if (SSL_write(ssl, send_buff, MSG_HEADERSIZE + total_size) < 0)
	{
		PLOG(WARNING) << "send socket error";
		exit(0);
	}
	// LOG(INFO) << "open dir:" << dir_buff;
}

void UserActions::Login()
{
	auto login_json = json::parse(recv_buff + MSG_HEADERSIZE);
	LOG(INFO) << "login json: " << login_json;

	string username = login_json["username"];
	string password = login_json["password"];
	json loginresp;

	json passwd;
	std::ifstream i("passwd");
	i >> passwd;
	if (passwd.contains(username) && passwd[username] == password)
	{
		std::string user_dir = username;
		user_path = "/" + user_dir + "/";

		cookie = randstr(20);
		current_username = username;
		loginresp["status"] = "success";
		loginresp["set-cookie"] = cookie;
		LOG(INFO) << "set cookie:" << cookie;
	}
	else
	{
		loginresp["status"] = "error";
	}

	int total_size = BuildJsonMsg(send_buff, LOGIN_RESPONSE, loginresp);
	if (SSL_write(ssl, send_buff, MSG_HEADERSIZE + total_size) < 0)
	{
		PLOG(WARNING) << "send socket error";
		exit(0);
	}
}

void UserActions::Register()
{
	auto regisinfo = json::parse(recv_buff + MSG_HEADERSIZE);
	LOG(INFO) << "register json: " << regisinfo;

	string username = regisinfo["username"];
	string password = regisinfo["password"];
	json regisresp;

	json passwd;
	std::ifstream i("passwd");
	i >> passwd;
	i.close();
	if (passwd.contains(username))
	{
		// passwd has same username
		regisresp["status"] = "error";
	}
	else
	{
		// append new user to passwd
		passwd[username] = password;
		std::ofstream i("passwd");
		i << passwd;
		i.close();
		std::string user_dir = base_path + username;
		if (access(user_dir.c_str(), 0) == -1)					  //如果文件夹不存在
			mkdir(user_dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR); //则创建
		user_path = "/" + username + "/";

		regisresp["status"] = "success";
	}

	int total_size = BuildJsonMsg(send_buff, LOGIN_RESPONSE, regisresp);
	if (SSL_write(ssl, send_buff, MSG_HEADERSIZE + total_size) < 0)
	{
		PLOG(WARNING) << "send socket error";
		exit(0);
	}
}
void KeepAlive()
{
}
void UserActions::DeleteAccount()
{
	json passwd;
	std::ifstream i("passwd");
	i >> passwd;
	i.close();

	auto idx = passwd.find(current_username);
	passwd.erase(idx);

	std::ofstream o("passwd");
	o << passwd;
	o.close();
}

int UserActions::ReadMsg()
{
	memset(recv_buff, 0, MAX_BUFF);

	int flag = SSL_read(ssl, recv_buff, MAX_BUFF);
	if (flag < 0)
	{
		PLOG(WARNING) << "recv socket error";
		return 0;
	}

	if (flag == 0)
	{
		return 0;
	}
	struct netmsg_header client_header;
	memcpy((void *)&client_header, (void *)recv_buff, MSG_HEADERSIZE);
	LOG(INFO) << "recv control msg: type:" << client_header.type << " size:" << client_header.size;

	client_initheader = client_header;

	// check cookie
	if ((client_header.type != LOGIN_REQUEST) && (client_header.type != REGISTER_REQUEST))
	{
		if (!CheckCookie(recv_buff))
		{
			return 0;
		}
	}

	switch (client_header.type)
	{
	case CATALOG_REQUEST:
		GetDirinfo();
		break;
	case FILE_REQUEST:
		SendFile();
		break;
	case FILE_UPLOAD:
		RecvFile();
		break;
	case FILE_DELETE:
		DeleteFile();
		break;
	case LOGIN_REQUEST:
		Login();
		break;
	case REGISTER_REQUEST:
		Register();
		break;
	case REGISTER_DELETE:
		DeleteAccount();
		break;
	case TCP_KEEPALIVE:
		KeepAlive();
		break;
	default:
		break;
	}

	return 1;
}