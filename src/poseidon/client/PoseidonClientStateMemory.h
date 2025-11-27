#ifndef POSEIDONCLIENTSTATEMEMORY_H
#define POSEIDONCLIENTSTATEMEMORY_H

#include "PoseidonClientState.h"

#include <unordered_map>

class PoseidonClientStateMemory : public PoseidonClientState {
public:
  PoseidonClientStateMemory() = default;

  ~PoseidonClientStateMemory() = default;

  bool Get(StateCell &out, const std::string &keyword) override;

  void Put(const StateCell &in, const std::string &keyword) override;

  void Clear() override;

  void DumpData(const std::string &dname = "poseidon_client_bak") override;

  void LoadData(const std::string &dname = "poseidon_client_bak") override;

  void GetKeywordsCnt(std::vector<int> &cnt) override;

private:
  std::unordered_map<std::string, StateCell> state_map;
};

#endif
