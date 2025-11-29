#ifndef CLIENTSTATE_MEMORY_H
#define CLIENTSTATE_MEMORY_H

#include "ClientState.h"
#include <string>
#include <unordered_map>
#include <vector>

class ClientStateMemory : public ClientState {
public:
  ClientStateMemory() = default;
  ~ClientStateMemory() override = default;

  bool Get(StateCell &out, const std::string &keyword) override;
  void Put(const StateCell &in, const std::string &keyword) override;
  void Clear() override;
  void DumpData(const std::string &dname = "bamboo_client_bak") override;
  void LoadData(const std::string &dname = "bamboo_client_bak") override;
  void GetKeywordsCnt(std::vector<int> &cnt) override;

private:
  std::unordered_map<std::string, StateCell> store_;
};

#endif