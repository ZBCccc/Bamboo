#ifndef CLIENTSTATE_SQL_H
#define CLIENTSTATE_SQL_H

#include "ClientState.h"
#include <string>
#include <vector>

extern "C" {
#include <sqlite3.h>
}

class ClientStateSQL : public ClientState
{
public:
    ClientStateSQL();
    ~ClientStateSQL() override;

    bool Get(StateCell &out, const std::string &keyword) override;
    void Put(const StateCell &in, const std::string &keyword) override;
    void Clear() override;
    void DumpData(const std::string &dname = "bamboo_client_bak") override;
    void LoadData(const std::string &dname = "bamboo_client_bak") override;
    void GetKeywordsCnt(std::vector<int> &cnt) override;

private:
    std::string db_path;
    sqlite3 *db{nullptr};
};

#endif