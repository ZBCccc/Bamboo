#ifndef SERVERSTORAGE_H
#define SERVERSTORAGE_H

#include <string>
#include <vector>

struct CDBCellX {
  std::string L;
  std::string D;
  std::string C;
};

struct CDBCellT {
  std::string addr;
  std::string val;
  std::string lastAddr;
  std::string alpha;
};

struct EDBCell {
  CDBCellX xCell;
  CDBCellT tCell;
};

class ServerStorage {
public:
  ServerStorage();

  virtual ~ServerStorage();

  virtual void Clear() = 0;

  virtual bool GetT(CDBCellT &in_out) = 0;

  virtual bool GetX(CDBCellX &in_out) = 0;

  virtual bool GetXSet(const std::string &xtag) = 0;

  virtual void Put(const EDBCell &out) = 0;

  virtual void PutXSet(const std::string &xtag) = 0;

  virtual void PopAllT(std::vector<CDBCellT> &cip_all) = 0;

  virtual void PopX(CDBCellX &in_out) = 0;

  virtual void PopAllX(std::vector<CDBCellX> &cip_all) = 0;

  virtual void PopAllXSet(std::vector<std::string> &xtags) = 0;

  virtual void PushBatchT(const std::vector<CDBCellT> &cip_all) = 0;

  virtual void PushBatchX(const std::vector<CDBCellX> &cip_all) = 0;

  virtual void DumpData(const std::string &dname) = 0;

  virtual void LoadData(const std::string &dname) = 0;
};

#endif
