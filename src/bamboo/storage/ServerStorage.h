#ifndef SERVERSTORAGE_H
#define SERVERSTORAGE_H

#include <string>
#include <vector>

struct EDBCell {
  std::string L;
  std::string D;
  std::string C;
};

class ServerStorage {
public:
  ServerStorage();

  virtual ~ServerStorage();

  virtual void Clear() = 0;

  virtual bool Get(EDBCell &in_out) = 0;

  virtual void Put(const EDBCell &out) = 0;

  virtual void PopAll(std::vector<EDBCell> &cip_all) = 0;

  virtual void PushBatch(const std::vector<EDBCell> &cip_all) = 0;

  virtual void DumpData(const std::string &dname) = 0;

  virtual void LoadData(const std::string &dname) = 0;
};

#endif
