#ifndef SERVERSTORAGEMEMORY_H
#define SERVERSTORAGEMEMORY_H

#include <string>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include "../ServerStorage.h"

class ServerStorageMemory : public ServerStorage
{
public:
    ServerStorageMemory();

    ~ServerStorageMemory();

    void Clear() override;

    bool GetT(CDBCellT &in_out) override;

    bool GetX(CDBCellX &in_out) override;

    void Put(const EDBCell &out) override;

    void PutXSet(const std::string &xtag) override;

    void PopAllT(std::vector<CDBCellT> &cip_all) override;

    void PopX(CDBCellX &in_out) override;

    bool GetXSet(const std::string &xtag) override;

    void PopAllX(std::vector<CDBCellX> &cip_all) override;

    void PopAllXSet(std::vector<std::string> &xtags) override;

    void PushBatchT(const std::vector<CDBCellT> &cip_all) override;

    void PushBatchX(const std::vector<CDBCellX> &cip_all) override;

    void DumpData(const std::string &dname) override;

    void LoadData(const std::string &dname) override;

private:
    std::unordered_map<std::string, CDBCellT> tStorage_;
    std::unordered_map<std::string, CDBCellX> xStorage_;
    std::unordered_set<std::string> xSet_;
};

#endif