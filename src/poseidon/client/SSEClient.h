#ifndef POSEIDON_SSECLIENT_H
#define POSEIDON_SSECLIENT_H

#include "PoseidonClient.h"
#include <string>
#include <vector>
#include <map>

class SSEClient
{
public:
    SSEClient() = delete;

    SSEClient(const std::string &srv_addr, int srv_port);

    void prepare_dataset(const std::map<std::string, std::vector<std::string>> &data_to_encrypt, PoseidonOp op = Poseidon_add);

    void Setup();

    void DataUpdate(const std::string &keyword, const std::string &id, PoseidonOp op);

    void Search(std::vector<std::string> &result, const std::vector<std::string> &keywords);

    void KeyUpdate(int thread_num = 1);

    void BackupDB(const std::string &name = "Backup");

    void LoadEDB(const std::string &name = "Backup");

private:
    void InitializeKey();

    std::string Encrypt_data(const std::string &data);
    std::string Decrypt_data(const std::string &data);

    PoseidonClient poseidon_client;
    unsigned char session_key[32];
    std::string server_addr;
    int server_port;
    int _ConnectToServer();
};

#endif
