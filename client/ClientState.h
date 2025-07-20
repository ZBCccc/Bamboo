#ifndef CLIENTSTATE_H
#define CLIENTSTATE_H

#include <string>
#include <vector>

extern "C"
{
#include <sqlite3.h>
};

struct StateCell
{
    std::string tk;
    int cntw;
};

class ClientState
{
public:
    ClientState();

    ~ClientState();

    // True if the data corresponding to the given keyword exists
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
