#include "ServerStorage.h"
#include <string>
#include <iostream>
extern "C"
{
#include "unistd.h"
}

using namespace std;

ServerStorage::ServerStorage() : conn{"postgresql://sse:123456@127.0.0.1/bamboo"}
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);

    w.exec0("CREATE TABLE IF NOT EXISTS cipher ("
            "L BYTEA PRIMARY KEY,"
            "D BYTEA NOT NULL,"
            "C BYTEA NOT NULL);");
    w.exec0("CREATE UNIQUE INDEX IF NOT EXISTS cipher_pkey ON cipher (l);");
    conn.prepare("get_data",
                 "SELECT D, C FROM cipher WHERE L=$1;");
    conn.prepare("put_data",
                 "INSERT INTO cipher (L,D,C) VALUES ($1,$2,$3);");
    conn.prepare("select_all",
                 "SELECT L,D,C FROM cipher;");

    w.commit();
}

ServerStorage::~ServerStorage()
{
    conn.close();
}

void ServerStorage::Clear()
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);

    w.exec0("DELETE FROM cipher;");

    w.commit();
}

bool ServerStorage::Get(EDBCell &in_out)
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);

    std::basic_string<std::byte> L_, bf;
    L_.assign((const std::byte *)in_out.L.c_str(), in_out.L.size());

    try
    {
        pqxx::row r = w.exec_prepared1("get_data", L_);

        bf = conn.unesc_bin(r[0].as<string>());
        in_out.D.assign((const char *)bf.c_str(), bf.size());

        bf = conn.unesc_bin(r[1].as<string>());
        in_out.C.assign((const char *)bf.c_str(), bf.size());

        return true;
    }
    catch (pqxx::unexpected_rows &e)
    {
        return false;
    }
}

void ServerStorage::Put(const EDBCell &in)
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);

    std::basic_string<std::byte> L_, D_, C_;

    L_.assign((const std::byte *)in.L.c_str(), in.L.size());
    D_.assign((const std::byte *)in.D.c_str(), in.D.size());
    C_.assign((const std::byte *)in.C.c_str(), in.C.size());

    try
    {
        w.exec_prepared0("put_data", L_, D_, C_);
        w.commit();
    }
    catch (std::exception &e)
    {
        cout << "Inertion error: " << e.what() << endl;
    }
}

void ServerStorage::PopAll(std::vector<EDBCell> &cip_all)
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);

    std::basic_string<std::byte> buf;

    try
    {
        pqxx::result r = w.exec_prepared("select_all");
        for (const auto &itr : r)
        {
            EDBCell cell;

            buf = conn.unesc_bin(itr[0].as<string>());
            cell.L.assign((const char *)buf.c_str(), buf.size());

            buf = conn.unesc_bin(itr[1].as<string>());
            cell.D.assign((const char *)buf.c_str(), buf.size());

            buf = conn.unesc_bin(itr[2].as<string>());
            cell.C.assign((const char *)buf.c_str(), buf.size());
            cip_all.emplace_back(cell);
        }
        w.exec0("DELETE FROM cipher;");
        w.commit();
    }
    catch (std::exception &e)
    {
        cout << "PopAll error: " << e.what() << endl;
    }
}

void ServerStorage::PushBatch(const std::vector<EDBCell> &cip_all)
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);
    std::basic_string<std::byte> L_, D_, C_;

    try
    {
        for (const auto &itr : cip_all)
        {
            L_.assign((const std::byte *)itr.L.c_str(), itr.L.size());
            D_.assign((const std::byte *)itr.D.c_str(), itr.D.size());
            C_.assign((const std::byte *)itr.C.c_str(), itr.C.size());
            w.exec_prepared0("put_data", L_, D_, C_);
        }
        w.commit();
    }
    catch (std::exception &e)
    {
        cout << "PushBatch error: " << e.what() << endl;
    }
}

void ServerStorage::DumpData(const std::string &dname)
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);

    try
    {
        w.exec0("DROP TABLE IF EXISTS " + dname + "_bak;");
        w.exec0("CREATE TABLE IF NOT EXISTS " + dname + "_bak AS (select * from cipher);");
        w.commit();
        sleep(15);
    }
    catch (std::exception &e)
    {
        cout << "DumpData error: " << e.what() << endl;
    }
}

void ServerStorage::LoadData(const std::string &dname)
{
    pqxx::robusttransaction<pqxx::serializable> w(conn);

    try
    {
        w.exec0("DROP TABLE IF EXISTS cipher;");
        w.exec0("CREATE TABLE IF NOT EXISTS cipher AS (SELECT * FROM " + dname + "_bak);");
        w.exec0("CREATE UNIQUE INDEX IF NOT EXISTS cipher_pkey ON cipher (l);");
        w.commit();
        sleep(15);
    }
    catch (std::exception &e)
    {
        cout << "LoadData error: " << e.what() << endl;
    }
}
