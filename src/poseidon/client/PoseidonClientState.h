#ifndef POSEIDONCLIENTSTATE_H
#define POSEIDONCLIENTSTATE_H

#include <string>
#include <vector>

struct StateCell {
  std::string tk;
  std::string rand;
  int cntw;
};

class PoseidonClientState {
public:
  PoseidonClientState() = default;

  virtual ~PoseidonClientState() = default;

  virtual bool Get(StateCell &out, const std::string &keyword) = 0;

  virtual void Put(const StateCell &in, const std::string &keyword) = 0;

  virtual void Clear() = 0;

  virtual void DumpData(const std::string &dname = "poseidon_client_bak") = 0;

  virtual void LoadData(const std::string &dname = "poseidon_client_bak") = 0;

  virtual void GetKeywordsCnt(std::vector<int> &cnt) = 0;
};

#endif
