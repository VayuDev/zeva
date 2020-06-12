#include "SftpClient.hpp"
#include <arpa/inet.h>
#include <cinttypes>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <Util.hpp>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <stdexcept>
#include <sys/types.h>
#include <trantor/utils/Logger.h>

static std::atomic<size_t> clients = 0;

SftpClient::SftpClient(const std::string &pUsername,
                       const std::string &pPassword,
                       const std::string &pHostname, int port) {
  LOG_DEBUG << "SftpClient: Connecting to " << pHostname << " on port " << port;
  int rc;
  struct sockaddr_in sin;
  const char *fingerprint;
  char *userauthlist;

  unsigned long hostaddr = inet_addr(pHostname.c_str());

  if (clients == 0) {
    rc = libssh2_init(0);
    if (rc != 0) {
      throw std::runtime_error{"libssh2 initialization failed (" +
                               std::to_string(rc) + ")"};
    }
  }

  sock = socket(AF_INET, SOCK_STREAM, 0);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = hostaddr;
  if (connect(sock, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in)) !=
      0) {
    close(sock);
    throw std::runtime_error{"Failed to connect"};
  }

  session = libssh2_session_init();

  if (!session) {
    close(sock);
    throw std::runtime_error("Unable to create session");
  }
  /* ... start it up. This will trade welcome banners, exchange keys,
   * and setup crypto, compression, and MAC layers
   */
  rc = libssh2_session_handshake(session, sock);

  if (rc) {
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
    throw std::runtime_error("Failure establishing SSH session: " +
                             std::to_string(rc));
  }

  /* At this point we havn't yet authenticated.  The first thing to do
   * is check the hostkey's fingerprint against our known hosts Your app
   * may have it hard coded, may go to a file, may present it to the
   * user, that's your call
   */
  fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);

  char buffer[128];
  memset(buffer, 0, sizeof(buffer));
  for (int i = 0; i < 20; i++) {
    snprintf(buffer + i * 3, 128 - i * 3 - 1, "%02X ",
             (unsigned char)fingerprint[i]);
  }
  LOG_INFO << "[SftpClient] Remote fingerprint is " << buffer;

  /* check what authentication methods are available */
  userauthlist =
      libssh2_userauth_list(session, pUsername.c_str(), pUsername.size());

  LOG_DEBUG << "SftpClient: Authentication methods: " << userauthlist;
  if (strstr(userauthlist, "password") == NULL) {
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
    throw std::runtime_error("Password authentication is disabled!");
  }

  /* We could authenticate via password */
  if (libssh2_userauth_password(session, pUsername.c_str(),
                                pPassword.c_str())) {
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
    throw std::runtime_error{"Authentication by password failed!\n"};
  }

  sftp_session = libssh2_sftp_init(session);

  if (!sftp_session) {
    libssh2_session_disconnect(session, "Normal Shutdown");
    libssh2_session_free(session);
    close(sock);
    throw std::runtime_error{"Unable to init SFTP session\n"};
  }

  /* Since we have not set non-blocking, tell libssh2 we are blocking */
  libssh2_session_set_blocking(session, 1);
  clients++;
}

SftpClient::~SftpClient() {
  libssh2_sftp_shutdown(sftp_session);
  libssh2_session_disconnect(session, "Normal Shutdown");
  libssh2_session_free(session);
  close(sock);
  if (--clients == 0) {
    libssh2_exit();
  }
}
std::vector<SftpFile> SftpClient::ls(const std::string &pPath) {
  auto handle = libssh2_sftp_opendir(sftp_session, pPath.c_str());
  if (!handle) {
    throw std::runtime_error("Unable to open directory");
  }
  std::vector<SftpFile> ret;
  do {
    char mem[FILENAME_MAX];
    char longentry[1];
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    /* loop until we fail */
    auto rc = libssh2_sftp_readdir_ex(handle, mem, sizeof(mem), longentry,
                                      sizeof(longentry), &attrs);
    if (rc > 0) {
      SftpFile file;
      file.fileSize = attrs.filesize;
      file.name.append(mem, rc);
      file.isDirectory = LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
      ret.emplace_back(std::move(file));
    } else {
      break;
    }
  } while (true);
  libssh2_sftp_close_handle(handle);
  return ret;
}
std::string SftpClient::download(const std::string &pPath) {
  auto handle =
      libssh2_sftp_open_ex(sftp_session, pPath.c_str(), pPath.size(),
                           LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
  if (!handle) {
    throw std::runtime_error("Unable to download: " + pPath);
  }
  LIBSSH2_SFTP_ATTRIBUTES attrs;
  if (libssh2_sftp_stat_ex(sftp_session, pPath.c_str(), pPath.size(), 0,
                           &attrs) < 0) {
    libssh2_sftp_close_handle(handle);
    throw std::runtime_error("Unable to stat: " + pPath);
  }

  char *buffer = (char *)malloc(attrs.filesize);
  uint64_t read = 0;
  while (read < attrs.filesize) {
    auto status =
        libssh2_sftp_read(handle, buffer + read, attrs.filesize - read);
    if (status < 0) {
      free(buffer);
      libssh2_sftp_close_handle(handle);
      throw std::runtime_error("Read failed");
    } else {
      read += status;
    }
  }
  libssh2_sftp_close_handle(handle);

  std::string ret(buffer, read);
  free(buffer);

  return ret;
}
