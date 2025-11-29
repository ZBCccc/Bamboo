#include "ServerStorageSQL.h"
#include <chrono>
#include <iostream>
#include <string>
extern "C" {
#include "unistd.h"
}

using namespace std;

ServerStorageSQL::ServerStorageSQL()
    : conn{"postgresql://sse:123456@127.0.0.1/bamboo"} {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  w.exec0("CREATE TABLE IF NOT EXISTS cipher ("
          "L BYTEA PRIMARY KEY,"
          "D BYTEA NOT NULL,"
          "C BYTEA NOT NULL);");
  w.exec0("CREATE UNIQUE INDEX IF NOT EXISTS cipher_pkey ON cipher (l);");
  conn.prepare("get_data", "SELECT D, C FROM cipher WHERE L=$1;");
  conn.prepare("put_data", "INSERT INTO cipher (L,D,C) VALUES ($1,$2,$3);");
  conn.prepare("select_all", "SELECT L,D,C FROM cipher;");

  w.commit();
}

ServerStorageSQL::~ServerStorageSQL() {}

void ServerStorageSQL::Clear() {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  w.exec0("DELETE FROM cipher;");

  w.commit();
}

bool ServerStorageSQL::Get(EDBCell &in_out) {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  const unsigned char *l_data =
      reinterpret_cast<const unsigned char *>(in_out.L.data());
  pqxx::binarystring L_bin{l_data, in_out.L.size()};

  try {
    pqxx::row r = w.exec_prepared1("get_data", L_bin);

    pqxx::binarystring d_bin{r[0]};
    in_out.D.assign(reinterpret_cast<const char *>(d_bin.data()), d_bin.size());

    pqxx::binarystring c_bin{r[1]};
    in_out.C.assign(reinterpret_cast<const char *>(c_bin.data()), c_bin.size());
    return true;
  } catch (pqxx::unexpected_rows &e) {
    return false;
  }
}

void ServerStorageSQL::Put(const EDBCell &in) {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  pqxx::binarystring L_bin{reinterpret_cast<const unsigned char *>(in.L.data()),
                           in.L.size()};
  pqxx::binarystring D_bin{reinterpret_cast<const unsigned char *>(in.D.data()),
                           in.D.size()};
  pqxx::binarystring C_bin{reinterpret_cast<const unsigned char *>(in.C.data()),
                           in.C.size()};

  try {
    w.exec_prepared0("put_data", L_bin, D_bin, C_bin);
    w.commit();
  } catch (std::exception &e) {
    cout << "Inertion error: " << e.what() << endl;
  }
}

void ServerStorageSQL::PopAll(std::vector<EDBCell> &cip_all) {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  try {
    pqxx::result r = w.exec_prepared("select_all");
    for (const auto &itr : r) {
      EDBCell cell;

      pqxx::binarystring l_bin{itr[0]};
      cell.L.assign(reinterpret_cast<const char *>(l_bin.data()), l_bin.size());

      pqxx::binarystring d_bin{itr[1]};
      cell.D.assign(reinterpret_cast<const char *>(d_bin.data()), d_bin.size());

      pqxx::binarystring c_bin{itr[2]};
      cell.C.assign(reinterpret_cast<const char *>(c_bin.data()), c_bin.size());
      cip_all.emplace_back(cell);
    }
    w.exec0("DELETE FROM cipher;");
    w.commit();
  } catch (std::exception &e) {
    cout << "PopAll error: " << e.what() << endl;
  }
}

void ServerStorageSQL::PushBatch(const std::vector<EDBCell> &cip_all) {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  try {
    for (const auto &itr : cip_all) {
      pqxx::binarystring L_bin{
          reinterpret_cast<const unsigned char *>(itr.L.data()), itr.L.size()};
      pqxx::binarystring D_bin{
          reinterpret_cast<const unsigned char *>(itr.D.data()), itr.D.size()};
      pqxx::binarystring C_bin{
          reinterpret_cast<const unsigned char *>(itr.C.data()), itr.C.size()};
      w.exec_prepared0("put_data", L_bin, D_bin, C_bin);
    }
    w.commit();
  } catch (std::exception &e) {
    cout << "PushBatch error: " << e.what() << endl;
  }
}

void ServerStorageSQL::DumpData(const std::string &dname) {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  try {
    w.exec0("DROP TABLE IF EXISTS " + dname + "_bak;");
    w.exec0("CREATE TABLE IF NOT EXISTS " + dname +
            "_bak AS (select * from cipher);");
    w.commit();
    sleep(15);
  } catch (std::exception &e) {
    cout << "DumpData error: " << e.what() << endl;
  }
}

void ServerStorageSQL::LoadData(const std::string &dname) {
  pqxx::robusttransaction<pqxx::serializable> w(conn);

  try {
    w.exec0("DROP TABLE IF EXISTS cipher;");
    w.exec0("CREATE TABLE IF NOT EXISTS cipher AS (SELECT * FROM " + dname +
            "_bak);");
    w.exec0("CREATE UNIQUE INDEX IF NOT EXISTS cipher_pkey ON cipher (l);");
    w.commit();
    sleep(15);
  } catch (std::exception &e) {
    cout << "LoadData error: " << e.what() << endl;
  }
}
