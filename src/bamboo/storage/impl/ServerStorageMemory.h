#ifndef SERVERSTORAGEMEMORY_H
#define SERVERSTORAGEMEMORY_H

#include <string>
#include <vector>
#include <unordered_map>
#include "../ServerStorage.h"

class ServerStorageMemory : public ServerStorage
{
public:
    ServerStorageMemory();

    ~ServerStorageMemory();

    void Clear() override;

    bool Get(EDBCell &in_out) override;

    void Put(const EDBCell &out) override;

    void PopAll(std::vector<EDBCell> &cip_all) override;

    void PushBatch(const std::vector<EDBCell> &cip_all) override;

    void DumpData(const std::string &dname) override;

    void LoadData(const std::string &dname) override;

private:
    std::unordered_map<std::string, EDBCell> storage_;
};

#endif