
#ifndef POSEIDONCLIENTSTATE_H
#define POSEIDONCLIENTSTATE_H

#include <string>
#include <vector>

extern "C"
{
#include <sqlite3.h>
};

struct StateCell
{
    std::string tk;
    std::string rand;
    int cntw;
};

class ClientState
{
public:
    ClientState();

    ~ClientState();

    bool Get(StateCell &out, const std::string &keyword);

    void Put(const StateCell &in, const std::string &keyword);

    void Clear();

    void DumpData(const std::string &dname = "bamboo_client_bak");

    void LoadData(const std::string &dname = "bamboo_client_bak");

    void GetKeywordsCnt(std::vector<int> &cnt);

private:
    std::string db_path;
    sqlite3 *db;
};

#endif
