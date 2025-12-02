#include "../client/PoseidonClient.h"
#include "../server/PoseidonServer.h"
#include "../storage/impl/ServerStorageMemory.h"
#include "../../core/primitive.h"
#include "metadata.h"
#include "relic/relic_core.h"
#include <iostream>
#include <vector>

int main() {
  // Initialize RELIC library before using any crypto functions
  core_init();
  ep_param_set(NIST_P256);
  std::cout << "Test starting..." << std::endl;
  PoseidonServer server;
  auto storage = std::make_unique<ServerStorageMemory>();
  server.SetStorage(std::move(storage));
  std::cout << "About to call server.Setup()" << std::endl;
  server.Setup();
  std::cout << "server.Setup() completed" << std::endl;

  PoseidonClient client;
  std::cout << "About to call client.Setup()" << std::endl;
  client.Setup();
  std::cout << "client.Setup() completed" << std::endl;

  std::vector<Metadata> metas;
  for (int i = 0; i < 200; i++) {
    Metadata meta;
    client.DataUpdate(meta, Poseidon_add, "abc", "file-" + std::to_string(i));
    metas.push_back(meta);
  }
  server.SaveBatch(metas);
  std::cout << "Encrypted " << 200 << " ciphers for abc" << std::endl;

  std::vector<Metadata> def_metas;
  for (int i = 0; i < 100; i++) {
    Metadata meta;
    client.DataUpdate(meta, Poseidon_add, "def", "file-" + std::to_string(i));
    def_metas.push_back(meta);
  }
  server.SaveBatch(def_metas);
  std::cout << "Encrypted " << 100 << " ciphers for def" << std::endl;

  TrapdoorMetadata td;
  client.Trapdoor(td, std::vector<std::string>({"def", "abc"}));

  std::cout << "Search all files" << std::endl;
  std::vector<ResMetadata> res;
  server.Search(res, td);
  std::cout << "Found " << res.size() << " files" << std::endl;

  core_clean();
  return 0;
}
