#ifndef SSESERVER_H
#define SSESERVER_H

#include "BambooServer.h"
#include <gmpxx.h>
extern "C" {
#include <relic/relic.h>
};

class SSEServer {
public:
  SSEServer() = delete;
  SSEServer(const std::string &addr, int port);

  void Run();

private:
  BambooServer bamboo_server;
  std::string server_addr;
  int server_port;
  unsigned char session_key[32];
  std::string Encrypt_data(const std::string &data);
  std::string Decrypt_data(const std::string &data);

  void _Setup(int sock);

  void _SaveCipher(int sock);

  void _SrchQry(int sock);

  void _KeyUpdt(int sock);

  void _ecdh(int sock);

  void _BackupEDB(int sock);

  void _LoadEDB(int sock);

  void _SaveBatch(int sock);

  int _ServerSockInit();
};

#endif
