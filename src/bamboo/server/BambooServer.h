#ifndef BambooSERVER_H
#define BambooSERVER_H

#include <string>
#include <vector>
#include <memory>
#include "../storage/ServerStorage.h"
#include "../storage/impl/ServerStorageMemory.h"

class BambooServer
{
public:
    BambooServer() = default;

    ~BambooServer() = default;

    void SetStorage(std::unique_ptr<ServerStorage> storage)
    {
        srv_store = std::move(storage);
    }

    void Setup();

    void Save(const std::string &L, const std::string &D, const std::string &C);

    void SaveBatch(const std::vector<std::string> &Ls, const std::vector<std::string> &Ds,
                   const std::vector<std::string> &Cs);

    void
    Search(std::vector<std::string> &result, const std::string &K_in, const std::string &L_in,
           const std::string &MskD_in, const std::string &MskC_in);

    void KeyUpdate(const std::string &token);

    void KeyUpdate_Parallel(const std::string &token, int num_threads);

    void DumpData(const std::string &name = "Backup");

    void LoadData(const std::string &name = "Backup");

private:
    std::unique_ptr<ServerStorage> srv_store;
};

void do_KeyUpdate_in_parallel(std::vector<EDBCell> &cells, int number, std::string &delta, int num_threads);

#endif
