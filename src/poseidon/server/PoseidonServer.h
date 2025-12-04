#ifndef POSEIDON_SERVER_H
#define POSEIDON_SERVER_H

#include "../storage/ServerStorage.h"
#include "../types/metadata.h"
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
};

class PoseidonServer {
public:
  PoseidonServer();

  ~PoseidonServer();

  void SetStorage(std::unique_ptr<ServerStorage> cdb) {
    _storage = std::move(cdb);
  }

  void Setup();

  void Save(const Metadata &meta);

  void SaveBatch(const std::vector<Metadata> &Metadatas);

  void Search(std::vector<ResMetadata> &result, const TrapdoorMetadata &td);

  void KeyUpdate(const std::string &token);

  void DumpData(const std::string &name = "Backup");

  void LoadData(const std::string &name = "Backup");

private:
  std::unique_ptr<ServerStorage> _storage;

  // Temporary variables for optimization
  bn_t K, c, e, d, ord;
  ep_t e_L, e_TD, e_TC, e_C, e_D;
  ep_t e_valTrap, e_alphaTrap, e_lastTrap;
  ep_t e_addr, e_val, e_lastAddr, e_alpha;
  ep_t e_xtk;
  // Buffer for serialization
  unsigned char buf[128];
};

#endif
