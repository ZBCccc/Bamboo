#ifndef SERVERSTORAGESQL_H
#define SERVERSTORAGESQL_H

#include <string>
#include <pqxx/pqxx>
#include "ServerStorage.h"

class ServerStorageSQL : public ServerStorage
{
public:
    ServerStorageSQL();

    ~ServerStorageSQL();

    void Clear() override;

    bool Get(EDBCell &in_out) override;

    void Put(const EDBCell &out) override;

    void PopAll(std::vector<EDBCell> &cip_all) override;

    void PushBatch(const std::vector<EDBCell> &cip_all) override;

    void DumpData(const std::string &dname) override;

    void LoadData(const std::string &dname) override;

private:
    pqxx::connection conn;
};

#endif
