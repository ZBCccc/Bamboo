#ifndef CLIENTSTATE_H
#define CLIENTSTATE_H

#include <string>
#include <vector>

struct StateCell {
  std::string tk;
  int cntw;
};

class ClientState {
public:
  ClientState() = default;
  virtual ~ClientState() = default;

  virtual bool Get(StateCell &out, const std::string &keyword) = 0;

  virtual void Put(const StateCell &in, const std::string &keyword) = 0;

  virtual void Clear() = 0;

  virtual void DumpData(const std::string &dname = "bamboo_client_bak") = 0;

  virtual void LoadData(const std::string &dname = "bamboo_client_bak") = 0;

  virtual void GetKeywordsCnt(std::vector<int> &cnt) = 0;
};

#endif
