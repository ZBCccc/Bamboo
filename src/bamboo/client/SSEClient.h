#ifndef SSECLIENT_H
#define SSECLIENT_H

#include "BambooClient.h"
#include <map>
#include <string>
#include <vector>

class SSEClient {
public:
  SSEClient() = delete;

  SSEClient(const std::string &srv_addr, int srv_port);

  void prepare_dataset(
      const std::map<std::string, std::vector<std::string>> &data_to_encrypt,
      BambooOp op = Bamboo_add);

  void Setup();

  void DataUpdate(const std::string &keyword, const std::string &id,
                  BambooOp op);

  void Search(std::vector<std::string> &result, const std::string &keyword);

  void KeyUpdate(int thread_num = 1);

  void BackupDB(const std::string &name = "Backup");

  void LoadEDB(const std::string &name = "Backup");

private:
  void InitializeKey();

  std::string Encrypt_data(const std::string &data);
  std::string Decrypt_data(const std::string &data);

  BambooClient bamboo_client;
  unsigned char session_key[32];
  std::string server_addr;
  int server_port;
  int _ConnectToServer();
};

#endif
