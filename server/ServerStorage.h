#ifndef SERVERSTORAGE_H
#define SERVERSTORAGE_H

#include <string>
#include <pqxx/pqxx>

struct EDBCell
{
    std::string L;
    std::string D;
    std::string C;
};

class ServerStorage
{
public:
    ServerStorage();

    ~ServerStorage();

    void Clear();

    bool Get(EDBCell &in_out);

    void Put(const EDBCell &out);

    void PopAll(std::vector<EDBCell> &cip_all);

    void PushBatch(const std::vector<EDBCell> &cip_all);

    void DumpData(const std::string &dname);

    void LoadData(const std::string &dname);

private:
    pqxx::connection conn;
};

#endif
