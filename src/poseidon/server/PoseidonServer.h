#ifndef POSEIDON_SERVER_H
#define POSEIDON_SERVER_H

#include "../storage/ServerStorage.h"
#include "../types/metadata.h"
#include <memory>
#include <string>
#include <vector>

class PoseidonServer {
public:
  PoseidonServer() = default;

  ~PoseidonServer() = default;

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
};

#endif
