#pragma once
#include <libssh2_sftp.h>
#include <string>
#include <vector>

class SftpFile {
public:
  std::string name;
  bool isDirectory = false;
  uint64_t fileSize;
};

class SftpClient {
public:
  SftpClient(const std::string &pUsername, const std::string &pPassword,
             const std::string &pHostname = "127.0.0.1", int port = 22);
  ~SftpClient();
  std::vector<SftpFile> ls(const std::string &pPath);
  std::string download(const std::string& pPath);
private:
  int sock;
  LIBSSH2_SFTP *sftp_session;
  LIBSSH2_SESSION *session;
};