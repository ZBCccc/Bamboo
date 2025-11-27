
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

  ~PoseidonClientState() = default;

  virtual bool Get(StateCell &out, const std::string &keyword);

  virtual void Put(const StateCell &in, const std::string &keyword);

  virtual void Clear();

  virtual void DumpData(const std::string &dname = "poseidon_client_bak");

  virtual void LoadData(const std::string &dname = "poseidon_client_bak");

  virtual void GetKeywordsCnt(std::vector<int> &cnt);
};

#endif
