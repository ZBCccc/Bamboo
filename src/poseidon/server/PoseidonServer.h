#ifndef PoseidonSERVER_H
#define PoseidonSERVER_H

#include "../storage/ServerStorage.h"
#include "../common/metadata.h"
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

  void KeyUpdate_Parallel(const std::string &token, int num_threads);

  void DumpData(const std::string &name = "Backup");

  void LoadData(const std::string &name = "Backup");

private:
  std::unique_ptr<ServerStorage> _storage;
};

void do_KeyUpdate_in_parallel(std::vector<EDBCell> &cells, int number,
                              std::string &delta, int num_threads);

#endif
